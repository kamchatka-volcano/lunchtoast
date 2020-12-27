#include "testlauncher.h"
#include "test.h"
#include "sectionsreader.h"
#include "utils.h"
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/algorithm/string.hpp>
#include <fstream>

namespace {
    std::string truncateString(std::string str, int maxWidth);
}

TestLauncher::TestLauncher(const fs::path& testPath,
                           const std::string& testFileExt,
                           const fs::path& reportFilePath,
                           const int reportWidth)
    : testPath_(testPath)
    , testFileExt_(testFileExt)
    , reportWidth_(reportWidth)
{
    initReporter(reportFilePath);
    collectTests(testPath);
}

bool TestLauncher::process()
{
    auto ok = processSuite("", defaultSuite_);
    for (auto& suitePair : suites_)
        if (!processSuite(truncateString(suitePair.first, reportWidth_ / 2), suitePair.second))
            ok = false;
    reportSummary();
    return ok;
}

bool TestLauncher::processSuite(const std::string& suiteName, TestSuite& suite)
{
    const auto& tests = suite.tests;
    const auto testsCount = static_cast<int>(tests.size());
    auto testNumber = 0;
    bool ok = true;
    for (const auto& testFile : tests){
        testNumber++;
        try{
            auto test = Test{testFile};
            auto result = test.process();
            if (result.type() == TestResultType::Success)
                suite.passedTestsCounter++;
            else
                ok = false;
            reportResult(test, result, suiteName, testNumber, testsCount);

        }
        catch(const TestConfigError& error){
            reportBrokenTest(testFile, error.what(), suiteName, testNumber, testsCount);
            ok = false;
        }
    }
    return ok;
}

void TestLauncher::collectTests(const fs::path &testPath)
{
    if (fs::is_directory(testPath)){
        if (testFileExt_.empty())
            throw std::runtime_error{"To launch all tests in the directory, test extension must be specified"};

        auto end = fs::directory_iterator{};
        for (auto it = fs::directory_iterator{testPath}; it != end; ++it)
            if (fs::is_directory(it->status()))
                collectTests(it->path());
            else if(it->path().extension().string() == testFileExt_)
                addTest(it->path());
    }
    else
        addTest(fs::absolute(testPath));
}

void TestLauncher::addTest(const fs::path &testFile)
{
    auto stream = std::ifstream{testFile.string()};
    auto suiteName = boost::trim_copy(readSectionValue(stream, "Suite"));
    processVariablesSubstitution(suiteName, testFile.stem().string(), testFile.parent_path().stem().string());
    if (suiteName.empty())
        defaultSuite_.tests.push_back(testFile);
    else
        suites_[suiteName].tests.push_back(testFile);
}

namespace{
std::string testResultStr(TestResultType resultType)
{
    switch(resultType){
    case TestResultType::Success: return "PASSED";
    case TestResultType::Failure: return "FAILED";
    case TestResultType::RuntimeError: return "ERROR";
    }
    return {};
}

template <typename... Args>
void print(Args&&... args)
{
    spdlog::trace(std::forward<Args>(args)...);
};

template <typename... Args>
void print(TestResultType type, Args&&... args)
{
    if (type == TestResultType::Success)
        spdlog::info(std::forward<Args>(args)...);
    else
        spdlog::error(std::forward<Args>(args)...);
};

void printNewLine()
{
    print(" ");
}

}

void TestLauncher::reportResult(const Test& test, const TestResult& result,
                                const std::string& suite, int suiteTestNumber, int suiteNumOfTests)
{
    auto header = fmt::format(" {} [ {} / {} ] ", suite, suiteTestNumber, suiteNumOfTests);
    if (suite.empty())
        header = header.substr(1);

    print(result.type(), "{:#^" + std::to_string(reportWidth_) + "}", header);
    print("Name: {}", test.name());
    if (result.type() != TestResultType::Success){
        if (!test.description().empty())
            print("Description: {}", test.description());
        if (!result.failedActionsMessages().empty())
            print(boost::join(result.failedActionsMessages(), "\n"));
    }
    if (result.type() == TestResultType::RuntimeError)
        if (!result.errorInfo().empty())
            print(result.errorInfo());
    auto resultStr = fmt::format("Result: {:>10}", testResultStr(result.type()));
    print(result.type(), "{:>"+ std::to_string(reportWidth_) + "}", resultStr);
}

void TestLauncher::reportBrokenTest(const fs::path& brokenTestConfig, const std::string& errorInfo,
                                    const std::string& suite, int suiteTestNumber, int suiteNumOfTests)
{
    auto header = fmt::format(" {} [ {} / {} ] ", suite, suiteTestNumber, suiteNumOfTests);
    if (suite.empty())
        header = header.substr(1);

    print(TestResultType::Failure, "{:#^" + std::to_string(reportWidth_) + "}", header);
    print("Test can't be started. Config file {} error:\n{}\n", brokenTestConfig.string(), errorInfo);
}

void TestLauncher::reportSuiteResult(std::string suiteName, int passedNumber, int totalNumber)
{
    auto width = reportWidth_ / 2 + 4;
    suiteName = truncateString(suiteName, width - 1) + ":";
    auto resultType = passedNumber == totalNumber ?
        TestResultType::Success : TestResultType::Failure;
    auto resultStr = fmt::format("{} out of {} passed, {} failed",
                                 passedNumber, totalNumber, totalNumber - passedNumber);
    print(resultType, "{:" + std::to_string(width) + "} {}", suiteName, resultStr);
}

void TestLauncher::reportSummary()
{
    auto totalTests = 0;
    auto totalPassed = 0;

    printNewLine();
    print("{:#^" + std::to_string(reportWidth_) + "}", "  SUMMARY  ");
    if (!defaultSuite_.tests.empty()){
        totalTests += defaultSuite_.tests.size();
        totalPassed += defaultSuite_.passedTestsCounter;
        reportSuiteResult("Default", defaultSuite_.passedTestsCounter, static_cast<int>(defaultSuite_.tests.size()));
    }
    for (const auto& suitePair : suites_){
        const auto& suite = suitePair.second;
        totalTests += suite.tests.size();
        totalPassed += suite.passedTestsCounter;
        reportSuiteResult(suitePair.first, suite.passedTestsCounter, static_cast<int>(suite.tests.size()));
    }
    print("---");
    reportSuiteResult("Total", totalPassed, totalTests);
}

void TestLauncher::initReporter(const boost::filesystem::path& reportFilePath)
{
    auto makeConsoleSink = []{
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
        sink->set_level(spdlog::level::trace);
        sink->set_color(spdlog::level::trace, "");
        sink->set_pattern("%^%v%$");
        return sink;
    };
    auto makeFileSink = [](const fs::path& logFilePath){
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(logFilePath.string(), true);
        sink->set_level(spdlog::level::trace);
        sink->set_pattern("%v");
        return sink;
    };

    auto logger = std::shared_ptr<spdlog::logger>{};
    if (reportFilePath.empty())
        logger = std::make_shared<spdlog::logger>("reporter", spdlog::sinks_init_list{makeConsoleSink()});
    else
        logger = std::make_shared<spdlog::logger>("reporter", spdlog::sinks_init_list{makeConsoleSink(), makeFileSink(reportFilePath)});

    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(logger);
}

namespace{
std::string truncateString(std::string str, int maxWidth)
{
    if (str.size() < static_cast<std::size_t>(4))
        return str;
    if (str.size() > static_cast<std::size_t>(maxWidth)){
        str.resize(static_cast<std::size_t>(maxWidth - 3));
        str += "...";
    }
    return str;
}
}

