#include "testlauncher.h"
#include "commandline.h"
#include "config.h"
#include "constants.h"
#include "errors.h"
#include "sectionsreader.h"
#include "test.h"
#include "testreporter.h"
#include "useraction.h"
#include "utils.h"
#include <figcone/configreader.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <sfun/utility.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>

namespace lunchtoast {
namespace views = ranges::views;

namespace {
std::vector<UserAction> makeUserActions(const Config& cfg)
{
    const auto toUserAction = [](const auto& action)
    {
        return UserAction{action};
    };
    return cfg.actions | views::transform(toUserAction) | ranges::to<std::vector>;
}

} //namespace

namespace fs = std::filesystem;
namespace views = ranges::views;

TestLauncher::TestLauncher(const TestReporter& reporter, const CommandLine& commandLine, const Config& cfg)
    : reporter_{reporter}
    , config_{cfg}
    , userActions_{makeUserActions(cfg)}
    , shellCommand_{commandLine.shell}
    , cleanup_{!commandLine.withoutCleanup}
    , selectedTags_{commandLine.select}
    , skippedTags_{commandLine.skip}
    , listOfFailedTests_{commandLine.listFailedTests}
    , dirWithFailedTests_{commandLine.collectFailedTests}
{
    collectTests(commandLine.testPath, {}, commandLine.searchDepth);
}

const TestReporter& TestLauncher::reporter() const
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
        stream << sfun::replace(sfun::path_string(path.parent_path()), "\\", "/") << std::endl;
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
    for (auto& [suiteName, suite] : suites_)
        concat(failedTests, processSuite(suiteName, suite));

    reporter().reportSummary(defaultSuite_, suites_);
    if (!listOfFailedTests_.get().empty())
        writePathList(failedTests, listOfFailedTests_);
    if (!dirWithFailedTests_.get().empty()) {
        copyDirList(failedTests, dirWithFailedTests_);
    }

    return failedTests.empty();
}

std::vector<std::filesystem::path> TestLauncher::processSuite(const std::string& suiteName, TestSuite& suite)
{
    const auto testsCount = std::ssize(suite.tests);
    auto testNumber = 0;
    auto failedTests = std::vector<fs::path>{};

    for (const auto& testCfg : suite.tests) {
        testNumber++;
        try {
            auto test = Test{testCfg.path, testCfg.vars, testCfg.userActions, shellCommand_, cleanup_};
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

void TestLauncher::collectTests(
        const fs::path& testPath,
        std::vector<fs::path> configList,
        std::optional<int> searchDirectoryLevels)
{
    if (fs::is_directory(testPath)) {
        if (fs::exists(testPath / hardcoded::configFilename))
            configList.emplace_back(testPath / hardcoded::configFilename);

        const auto end = fs::directory_iterator{};
        auto dirSet = std::set<fs::path>{};
        auto fileSet = std::set<fs::path>{};
        for (auto it = fs::directory_iterator{testPath}; it != end; ++it)
            if (fs::is_directory(it->status()))
                dirSet.insert(it->path());
            else if (sfun::path_string(it->path().filename()) == hardcoded::testCaseFilename)
                fileSet.insert(it->path());

        for (const auto& filePath : fileSet)
            addTest(fs::canonical(filePath), configList);

        if (!searchDirectoryLevels.has_value() || searchDirectoryLevels > 0)
            for (const auto& dirPath : dirSet)
                collectTests(
                        dirPath,
                        configList,
                        searchDirectoryLevels.has_value() ? searchDirectoryLevels.value() - 1 : searchDirectoryLevels);
    }
    else
        addTest(fs::canonical(testPath), configList);
}

namespace {
std::string getSectionValue(std::string_view sectionName, const std::vector<lunchtoast::Section>& sections)
{
    auto it = std::ranges::find_if(
            sections,
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
        const std::string& varDirName)
{
    auto vars = std::unordered_map<std::string, std::string>{};
    vars["DIR"] = varDirName;
    for (const auto& [varName, varValue] : config.vars)
        vars[varName] = varValue;
    for (const auto& tag : tags) {
        auto itTagVars = std::ranges::find_if(
                config.tagVars,
                [&](const auto& tagVarsSet)
                {
                    return tagVarsSet.tag == tag;
                });
        if (itTagVars != config.tagVars.end())
            for (const auto& [varName, varValue] : itTagVars->vars)
                vars[varName] = varValue;
    }
    return vars;
}

std::unordered_map<std::string, std::string> makeTestVariables(
        const std::vector<fs::path>& configList,
        const std::set<std::string>& tags,
        const std::string& varDirName)
{
    auto result = std::unordered_map<std::string, std::string>{};
    auto configReader = figcone::ConfigReader{};
    for (const auto& configPath : configList) {
        auto cfg = configReader.readShoalFile<Config>(configPath);
        const auto cfgVars = makeTestVariables(cfg, tags, varDirName);
        for (const auto& [cfgVarName, cfgVarValue] : cfgVars)
            result.insert_or_assign(cfgVarName, cfgVarValue);
    }
    return result;
}

std::vector<UserAction> makeUserActions(const std::vector<std::filesystem::path>& cfgList)
{
    const auto readConfigActions = [](const fs::path& configPath)
    {
        auto configReader = figcone::ConfigReader{};
        auto cfg = configReader.readShoalFile<Config>(configPath);
        return makeUserActions(cfg);
    };
    return cfgList | //
            views::reverse | //
            views::transform(readConfigActions) | //
            views::join | //
            ranges::to<std::vector>;
}

} //namespace

void TestLauncher::addTest(const fs::path& testFile, const std::vector<std::filesystem::path>& configList)
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

    const auto testVars = [&]
    {
        auto result = makeTestVariables(
                configList,
                tagsSet,
                sfun::path_string(testFile.parent_path().stem()));
        auto cmdLineConfigVariables = makeTestVariables(
                config_,
                tagsSet,
                sfun::path_string(testFile.parent_path().stem()));
        std::ranges::copy(cmdLineConfigVariables, std::inserter(result, result.begin()));
        return result;
    }();

    const auto userActions = [&]
    {
        auto result = makeUserActions(configList);
        std::ranges::copy(userActions_.get(), std::inserter(result, result.begin()));
        return result;
    }();

    const auto suiteName = processVariablesSubstitution(getSectionValue("Suite", sections), testVars);
    if (suiteName.empty()) {
        defaultSuite_.tests.push_back({testFile, isEnabled, testVars, userActions});
        if (!isEnabled)
            defaultSuite_.disabledTestsCounter++;
    }
    else {
        suites_[suiteName].tests.push_back({testFile, isEnabled, testVars, userActions});
        if (!isEnabled)
            suites_[suiteName].disabledTestsCounter++;
    }
}

} //namespace lunchtoast