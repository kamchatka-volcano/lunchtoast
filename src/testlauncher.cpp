#include "testlauncher.h"
#include "commandline.h"
#include "config.h"
#include "errors.h"
#include "sectionsreader.h"
#include "test.h"
#include "testreporter.h"
#include "useraction.h"
#include "utils.h"
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <sfun/utility.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>

namespace lunchtoast {

namespace {
std::vector<UserAction> makeUserActions(const Config& cfg)
{
    auto result = std::vector<UserAction>{};
    std::transform(
            cfg.actions.cbegin(),
            cfg.actions.cend(),
            std::back_inserter(result),
            [](const auto& action)
            {
                return UserAction{action};
            });
    return result;
}
} //namespace

namespace fs = std::filesystem;

TestLauncher::TestLauncher(const TestReporter& reporter, const CommandLine& commandLine, const Config& cfg)
    : reporter_{reporter}
    , config_{cfg}
    , userActions_{makeUserActions(cfg)}
    , shellCommand_{commandLine.shell}
    , cleanup_{!commandLine.noCleanup}
    , selectedTags_{commandLine.select}
    , skippedTags_{commandLine.skip}
    , listOfFailedTests_{commandLine.listFailedTests}
    , dirWithFailedTests_{commandLine.collectFailedTests}
{
    collectTests(commandLine.testPath, commandLine.ext);
}

const TestReporter& TestLauncher::reporter()
{
    return reporter_;
}

namespace {
template<typename TContainer>
auto concat(TContainer& lhs, const TContainer& rhs)
{
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
}

void writePathList(const std::vector<fs::path>& pathList, const fs::path& outputFile)
{
    if (pathList.empty())
        return;

    auto stream = std::ofstream{outputFile};
    for (const auto& path : pathList)
        stream << sfun::replace(sfun::pathString(path), "\\", "/") << std::endl;
}

void copyDirList(const std::vector<fs::path>& pathList, const fs::path& targetDir)
{
    if (pathList.empty())
        return;

    if (!fs::exists(targetDir))
        fs::create_directory(targetDir);

    for (const auto& path : pathList)
        if (fs::is_directory(path))
            fs::copy(path, targetDir / path.stem(), fs::copy_options::update_existing | fs::copy_options::recursive);
        else
            fs::copy(
                    path.parent_path(),
                    targetDir / path.parent_path().stem(),
                    fs::copy_options::update_existing | fs::copy_options::recursive);
}

} //namespace

bool TestLauncher::process()
{
    auto failedTests = std::vector<fs::path>{};
    concat(failedTests, processSuite("", defaultSuite_));
    for (auto& suitePair : suites_)
        concat(failedTests, processSuite(suitePair.first, suitePair.second));

    reporter().reportSummary(defaultSuite_, suites_);
    if (!listOfFailedTests_.empty())
        writePathList(failedTests, listOfFailedTests_);
    if (!dirWithFailedTests_.empty()) {
        copyDirList(failedTests, dirWithFailedTests_);
    }

    return failedTests.empty();
}

std::vector<std::filesystem::path> TestLauncher::processSuite(const std::string& suiteName, TestSuite& suite)
{
    const auto testsCount = sfun::ssize(suite.tests);
    auto testNumber = 0;
    auto failedTests = std::vector<fs::path>{};

    for (const auto& testCfg : suite.tests) {
        testNumber++;
        try {
            auto test = Test{testCfg.path, testCfg.vars, userActions_, shellCommand_, cleanup_};
            if (!testCfg.isEnabled) {
                reporter().reportDisabledTest(test, suiteName, testNumber, testsCount);
                continue;
            }
            auto result = test.process();
            if (result.type() == TestResultType::Success)
                suite.passedTestsCounter++;
            else
                failedTests.push_back(testCfg.path);

            reporter().reportResult(test, result, suiteName, testNumber, testsCount);
        }
        catch (const TestConfigError& error) {
            reporter().reportBrokenTest(testCfg.path, error.what(), suiteName, testNumber, testsCount);
            failedTests.push_back(testCfg.path);
        }
    }
    return failedTests;
}

void TestLauncher::collectTests(const fs::path& testPath, const std::string& testFileExt)
{
    if (fs::is_directory(testPath)) {
        if (testFileExt.empty())
            throw std::runtime_error{"To launch all tests in the directory, test extension must be specified"};

        auto end = fs::directory_iterator{};
        auto dirSet = std::set<fs::path>{};
        auto fileSet = std::set<fs::path>{};
        for (auto it = fs::directory_iterator{testPath}; it != end; ++it)
            if (fs::is_directory(it->status()))
                dirSet.insert(it->path());
            else if (sfun::pathString(it->path().extension()) == testFileExt)
                fileSet.insert(it->path());

        for (const auto& dirPath : dirSet)
            collectTests(dirPath, testFileExt);
        for (const auto& filePath : fileSet)
            addTest(fs::canonical(filePath));
    }
    else
        addTest(fs::canonical(testPath));
}

namespace {
std::string getSectionValue(std::string_view sectionName, const std::vector<lunchtoast::Section>& sections)
{
    auto it = std::find_if(
            sections.begin(),
            sections.end(),
            [&](const auto& section)
            {
                return section.name == sectionName;
            });
    if (it != sections.end())
        return it->value;
    return {};
}

bool isTestSelected(
        const std::set<std::string>& testTags,
        const std::vector<std::string>& selectedTags,
        const std::vector<std::string>& skippedTags)
{
    auto isTestTaggedWith = [&](const std::string& tag)
    {
        return testTags.count(tag);
    };
    if (!selectedTags.empty())
        if (std::none_of(selectedTags.begin(), selectedTags.end(), isTestTaggedWith))
            return false;
    return std::none_of(skippedTags.begin(), skippedTags.end(), isTestTaggedWith);
}

std::unordered_map<std::string, std::string> makeTestVariables(
        const Config& config,
        const std::set<std::string>& tags,
        const std::string& varFileName,
        const std::string& varDirName)
{
    auto vars = std::unordered_map<std::string, std::string>{};
    vars["FILENAME"] = varFileName;
    vars["DIR"] = varDirName;
    for (const auto& [varName, varValue] : config.vars)
        vars[varName] = varValue;
    for (const auto& tag : tags) {
        auto itTagVars = std::find_if(
                config.tagVars.cbegin(),
                config.tagVars.cend(),
                [&](const auto& tagVarsSet)
                {
                    return tagVarsSet.tag == tag;
                });
        if (itTagVars != config.tagVars.cend())
            for (const auto& [varName, varValue] : itTagVars->vars)
                vars[varName] = varValue;
    }
    return vars;
}

} //namespace

void TestLauncher::addTest(const fs::path& testFile)
{
    auto stream = std::ifstream{testFile, std::ios::binary};
    auto error = SectionReadingError{};
    auto sections = lunchtoast::readSections(stream, error);

    auto enabledStr = toLower(getSectionValue("Enabled", sections));
    auto isEnabled = (enabledStr.empty() || enabledStr == "true");
    stream.clear();
    stream.seekg(0, std::ios::beg);

    auto tagsStr = getSectionValue("Tags", sections);
    auto tags = sfun::split(tagsStr, ",");
    auto tagsSet = std::set<std::string>{tags.begin(), tags.end()};
    if (!isTestSelected(tagsSet, selectedTags_, skippedTags_))
        return;

    const auto testVars = makeTestVariables(
            config_,
            tagsSet,
            sfun::pathString(testFile.stem()),
            sfun::pathString(testFile.parent_path().stem()));

    const auto suiteName = processVariablesSubstitution(getSectionValue("Suite", sections), testVars);
    if (suiteName.empty()) {
        defaultSuite_.tests.push_back({testFile, isEnabled, testVars});
        if (!isEnabled)
            defaultSuite_.disabledTestsCounter++;
    }
    else {
        suites_[suiteName].tests.push_back({testFile, isEnabled, testVars});
        if (!isEnabled)
            suites_[suiteName].disabledTestsCounter++;
    }
}

} //namespace lunchtoast