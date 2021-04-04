#include "testreporter.h"
#include "test.h"
#include "testresult.h"
#include "string_utils.h"
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/algorithm/string.hpp>

TestReporter::TestReporter(const fs::path& reportFilePath,
                           const int reportWidth)
    : reportWidth_(reportWidth)
{
    initReporter(reportFilePath);
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

void TestReporter::reportResult(const Test& test, const TestResult& result,
                                std::string suiteName, int suiteTestNumber, int suiteNumOfTests) const
{
    suiteName = truncateString(suiteName, reportWidth_ / 2);
    auto header = fmt::format(" {} [ {} / {} ] ", suiteName, suiteTestNumber, suiteNumOfTests);
    if (suiteName.empty())
        header = header.substr(1);

    print(result.type(), "{:#^" + std::to_string(reportWidth_) + "}", header);
    print("Name: {}", test.name());
    if (result.type() != TestResultType::Success){
        if (!test.description().empty()){
            auto descriptionHasMultipleLines = !str::after(test.description(), "\n").empty();
            if (descriptionHasMultipleLines)
                print("Description:\n{}", test.description());
            else
                print("Description: {}", test.description());
        }
        if (!result.failedActionsMessages().empty()){
            if (result.failedActionsMessages().size() > 1)
                print("Failure:\n{}", boost::join(result.failedActionsMessages(), "\n"));
            else
                print("Failure: {}", boost::join(result.failedActionsMessages(), "\n"));
        }
    }
    if (result.type() == TestResultType::RuntimeError)
        if (!result.errorInfo().empty())
            print(result.errorInfo());
    const auto resultStr = fmt::format("Result: {:>10}", testResultStr(result.type()));
    print(result.type(), "{:>"+ std::to_string(reportWidth_) + "}", resultStr);
}

void TestReporter::reportBrokenTest(const fs::path& brokenTestConfig, const std::string& errorInfo,
                                    std::string suiteName, int suiteTestNumber, int suiteNumOfTests) const
{
    suiteName = truncateString(suiteName, reportWidth_ / 2);
    auto header = fmt::format(" {} [ {} / {} ] ", suiteName, suiteTestNumber, suiteNumOfTests);
    if (suiteName.empty())
        header = header.substr(1);

    print(TestResultType::Failure, "{:#^" + std::to_string(reportWidth_) + "}", header);
    print("Test can't be started. Config file {} error:\n{}\n", brokenTestConfig.string(), errorInfo);
}

void TestReporter::reportDisabledTest(const Test& test,
                      std::string suiteName, int suiteTestNumber, int suiteNumOfTests) const
{
    suiteName = truncateString(suiteName, reportWidth_ / 2);
    auto header = fmt::format(" {} [ {} / {} ] ", suiteName, suiteTestNumber, suiteNumOfTests);
    if (suiteName.empty())
        header = header.substr(1);
    print("{:#^" + std::to_string(reportWidth_) + "}", header);
    print("Name: {}", test.name());

    if (!test.description().empty()){
        auto descriptionHasMultipleLines = !str::after(test.description(), "\n").empty();
        if (descriptionHasMultipleLines)
            print("Description:\n{}", test.description());
        else
            print("Description: {}", test.description());
    }

    const auto resultStr = fmt::format("Result: {:>10}", "DISABLED");
    print("{:>"+ std::to_string(reportWidth_) + "}", resultStr);
}

void TestReporter::reportSuiteResult(std::string suiteName, int passedNumber, int totalNumber, int disabledNumber) const
{
    if (totalNumber == 0 && disabledNumber == 0)
        return;
    const auto failedNumber = totalNumber - disabledNumber - passedNumber;
    const auto width = reportWidth_ / 2 + 4;
    suiteName = truncateString(suiteName, width - 1) + ":";
    const auto resultType = failedNumber ?
        TestResultType::Failure : TestResultType::Success;
    auto resultStr = std::string{};
    if (disabledNumber)
        resultStr = fmt::format("{} out of {} passed, {} failed, {} disabled",
                                passedNumber, totalNumber, failedNumber, disabledNumber);
    else
        resultStr = fmt::format("{} out of {} passed, {} failed",
                                passedNumber, totalNumber, failedNumber);
    print(resultType, "{:" + std::to_string(width) + "} {}", suiteName, resultStr);
}

namespace{
std::tuple<int, int, int> countTotals(const TestSuite& defaultSuite, const std::map<std::string, TestSuite>& suites)
{
    auto totalTests = defaultSuite.tests.size();
    auto totalPassed = defaultSuite.passedTestsCounter;
    auto totalDisabled = defaultSuite.disabledTestsCounter;
    for (const auto& suitePair : suites){
        const auto& suite = suitePair.second;
        totalTests += suite.tests.size();
        totalPassed += suite.passedTestsCounter;
        totalDisabled += suite.disabledTestsCounter;
    }
    return std::make_tuple(totalTests, totalPassed, totalDisabled);
}
}

void TestReporter::reportSummary(const TestSuite& defaultSuite, const std::map<std::string, TestSuite>& suites) const
{
    auto[totalTests, totalPassed, totalDisabled] = countTotals(defaultSuite, suites);
    if (totalTests == 0 && totalDisabled == 0){
        print("No tests were found. Exiting.");
        return;
    }

    if (totalTests)
        printNewLine();
    print("{:#^" + std::to_string(reportWidth_) + "}", "  SUMMARY  ");
    reportSuiteResult("Default",
                      defaultSuite.passedTestsCounter,
                      static_cast<int>(defaultSuite.tests.size()),
                      defaultSuite.disabledTestsCounter);
    for (const auto& suitePair : suites){
        const auto& suite = suitePair.second;
        reportSuiteResult(suitePair.first,
                          suite.passedTestsCounter,
                          static_cast<int>(suite.tests.size()),
                          suite.disabledTestsCounter);
    }
    print("---");
    reportSuiteResult("Total", totalPassed, totalTests, totalDisabled);
}

void TestReporter::initReporter(const fs::path& reportFilePath)
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
