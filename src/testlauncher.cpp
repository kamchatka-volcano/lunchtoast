#include "testlauncher.h"
#include "errors.h"
#include "sectionsreader.h"
#include "test.h"
#include "testreporter.h"
#include "utils.h"
#include <sfun/string_utils.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>

namespace lunchtoast {
namespace fs = std::filesystem;

TestLauncher::TestLauncher(
        const fs::path& testPath,
        const std::string& testFileExt,
        std::string shellCommand,
        bool cleanup,
        const TestReporter& reporter,
        std::vector<std::string> selectedTags,
        std::vector<std::string> skippedTags)
    : reporter_{reporter}
    , shellCommand_{std::move(shellCommand)}
    , cleanup_{cleanup}
    , selectedTags_{std::move(selectedTags)}
    , skippedTags_{std::move(skippedTags)}
{
    collectTests(testPath, testFileExt);
}

const TestReporter& TestLauncher::reporter()
{
    return reporter_;
}

bool TestLauncher::process()
{
    auto ok = processSuite("", defaultSuite_);
    for (auto& suitePair : suites_)
        if (!processSuite(suitePair.first, suitePair.second))
            ok = false;
    reporter().reportSummary(defaultSuite_, suites_);
    return ok;
}

bool TestLauncher::processSuite(const std::string& suiteName, TestSuite& suite)
{
    const auto& tests = suite.tests;
    const auto testsCount = static_cast<int>(tests.size());
    auto testNumber = 0;
    bool ok = true;
    for (const auto& testCfg : tests) {
        testNumber++;
        try {
            auto test = Test{testCfg.path, shellCommand_, cleanup_};
            if (!testCfg.isEnabled) {
                reporter().reportDisabledTest(test, suiteName, testNumber, testsCount);
                continue;
            }
            auto result = test.process();
            if (result.type() == TestResultType::Success)
                suite.passedTestsCounter++;
            else
                ok = false;
            reporter().reportResult(test, result, suiteName, testNumber, testsCount);
        }
        catch (const TestConfigError& error) {
            reporter().reportBrokenTest(testCfg.path, error.what(), suiteName, testNumber, testsCount);
            ok = false;
        }
    }
    return ok;
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
            else if (it->path().extension().string() == testFileExt)
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

} //namespace

void TestLauncher::addTest(const fs::path& testFile)
{
    auto stream = std::ifstream{testFile.string()};
    auto error = SectionReadingError{};
    auto sections = lunchtoast::readSections(stream, error);

    auto enabledStr = toLower(getSectionValue("Enabled", sections));
    auto isEnabled = (enabledStr.empty() || enabledStr == "true");
    stream.clear();
    stream.seekg(0, std::ios::beg);
    auto suiteName = getSectionValue("Suite", sections);
    processVariablesSubstitution(suiteName, testFile.stem().string(), testFile.parent_path().stem().string());

    auto tagsStr = getSectionValue("Tags", sections);
    auto tags = sfun::split(tagsStr, ",");
    auto tagsSet = std::set<std::string>{tags.begin(), tags.end()};

    if (!isTestSelected(tagsSet, selectedTags_, skippedTags_))
        return;

    if (suiteName.empty()) {
        defaultSuite_.tests.push_back({testFile, isEnabled});
        if (!isEnabled)
            defaultSuite_.disabledTestsCounter++;
    }
    else {
        suites_[suiteName].tests.push_back({testFile, isEnabled});
        if (!isEnabled)
            suites_[suiteName].disabledTestsCounter++;
    }
}

} //namespace lunchtoast