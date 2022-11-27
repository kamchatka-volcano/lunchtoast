#include "testlauncher.h"
#include "testreporter.h"
#include "test.h"
#include "sectionsreader.h"
#include "utils.h"
#include "errors.h"
#include <boost/algorithm/string.hpp>
#include <range/v3/algorithm.hpp>
#include <fstream>


namespace lunchtoast{

TestLauncher::TestLauncher(const fs::path& testPath,
                           const std::string& testFileExt,
                           const TestReporter& reporter)
        : reporter_(reporter)
{
    collectTests(testPath, testFileExt);
}

bool TestLauncher::process()
{
    auto ok = processSuite("", defaultSuite_);
    for (auto& suitePair: suites_)
        if (!processSuite(suitePair.first, suitePair.second))
            ok = false;
    reporter_.reportSummary(defaultSuite_, suites_);
    return ok;
}

bool TestLauncher::processSuite(const std::string& suiteName, TestSuite& suite)
{
    const auto& tests = suite.tests;
    const auto testsCount = static_cast<int>(tests.size());
    auto testNumber = 0;
    bool ok = true;
    for (const auto& testCfg: tests){
        testNumber++;
        try{
            auto test = Test{testCfg.path};
            if (!testCfg.isEnabled){
                reporter_.reportDisabledTest(test, suiteName, testNumber, testsCount);
                continue;
            }
            auto result = test.process();
            if (result.type() == TestResultType::Success)
                suite.passedTestsCounter++;
            else
                ok = false;
            reporter_.reportResult(test, result, suiteName, testNumber, testsCount);

        }
        catch (const TestConfigError& error){
            reporter_.reportBrokenTest(testCfg.path, error.what(), suiteName, testNumber, testsCount);
            ok = false;
        }
    }
    return ok;
}

void TestLauncher::collectTests(const fs::path& testPath, const std::string& testFileExt)
{
    if (fs::is_directory(testPath)){
        if (testFileExt.empty())
            throw std::runtime_error{"To launch all tests in the directory, test extension must be specified"};

        auto end = fs::directory_iterator{};
        for (auto it = fs::directory_iterator{testPath}; it != end; ++it)
            if (fs::is_directory(it->status()))
                collectTests(it->path(), testFileExt);
            else if (it->path().extension().string() == testFileExt)
                addTest(fs::canonical(it->path()));
    } else
        addTest(fs::canonical(testPath));
}

namespace{
std::string getSectionValue(std::string_view sectionName, const std::vector<lunchtoast::Section>& sections)
{
    auto it = ranges::find_if(sections, [&](const auto& section){ return section.name == sectionName; });
    if (it != sections.end())
        return it->value;
    return {};
}

}

void TestLauncher::addTest(const fs::path& testFile)
{
    auto stream = std::ifstream{testFile.string()};
    auto error = SectionReadingError{};
    auto sections = lunchtoast::readSections(stream, error);

    auto enabledStr = boost::to_lower_copy(getSectionValue("Enabled", sections));
    auto isEnabled = (enabledStr.empty() || enabledStr == "true");
    stream.clear();
    stream.seekg(0, std::ios::beg);
    auto suiteName = getSectionValue("Suite", sections);
    processVariablesSubstitution(suiteName, testFile.stem().string(), testFile.parent_path().stem().string());

    if (suiteName.empty()){
        defaultSuite_.tests.push_back({testFile, isEnabled});
        if (!isEnabled)
            defaultSuite_.disabledTestsCounter++;
    } else{
        suites_[suiteName].tests.push_back({testFile, isEnabled});
        if (!isEnabled)
            suites_[suiteName].disabledTestsCounter++;
    }
}

}