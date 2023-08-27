#include "testreporter.h"
#include "test.h"
#include "testresult.h"
#include "utils.h"
#include <range/v3/view.hpp>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <sfun/utility.h>
#include <spdlog/logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace lunchtoast {
namespace views = ranges::views;
namespace fs = std::filesystem;

namespace {
std::string testResultStr(TestResultType resultType)
{
    switch (resultType) {
    case TestResultType::Success:
        return "PASSED";
    case TestResultType::Failure:
        return "FAILED";
    case TestResultType::RuntimeError:
        return "ERROR";
    }
    return {};
}

template<typename... Args>
void print(fmt::format_string<Args...> s, Args&&... args)
{
    spdlog::trace(s, std::forward<Args>(args)...);
}

template<typename... Args>
void print(TestResultType type, fmt::format_string<Args...> s, Args&&... args)
{
    if (type == TestResultType::Success)
        spdlog::info(s, std::forward<Args>(args)...);
    else
        spdlog::error(s, std::forward<Args>(args)...);
}

void printNewLine()
{
    print(" ");
}

std::string truncateString(std::string str, int maxWidth)
{
    if (std::ssize(str) < 4)
        return str;
    if (std::ssize(str) > maxWidth) {
        str.resize(static_cast<std::size_t>(maxWidth - 3));
        str += "...";
    }
    return str;
}

void initReporter(const fs::path& reportFilePath)
{
    auto makeConsoleSink = []
    {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
        sink->set_level(spdlog::level::trace);
        sink->set_pattern("%^%v%$");
        return sink;
    };
    auto makeFileSink = [](const fs::path& logFilePath)
    {
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(sfun::path_string(logFilePath), true);
        sink->set_level(spdlog::level::trace);
        sink->set_pattern("%v");
        return sink;
    };

    auto logger = std::shared_ptr<spdlog::logger>{};

    if (reportFilePath.empty())
        logger = std::make_shared<spdlog::logger>("reporter", spdlog::sinks_init_list{makeConsoleSink()});

    else
        logger = std::make_shared<spdlog::logger>(
                "reporter",
                spdlog::sinks_init_list{makeConsoleSink(), makeFileSink(reportFilePath)});

    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(logger);
}

} //namespace

TestReporter::TestReporter(const fs::path& reportFilePath, int reportWidth)
    : reportWidth_(reportWidth)
{
    initReporter(reportFilePath);
}

void TestReporter::reportResult(
        const Test& test,
        const TestResult& result,
        std::string suiteName,
        int suiteTestNumber,
        sfun::ssize_t suiteNumOfTests) const
{
    suiteName = truncateString(suiteName, reportWidth_ / 2);
    auto header = fmt::format(" {} [ {} / {} ] ", suiteName, suiteTestNumber, suiteNumOfTests);
    if (suiteName.empty())
        header = header.substr(1);

    print(result.type(), fmt::runtime("{:#^" + std::to_string(reportWidth_) + "}"), header);
    print("Name: {}", test.name());
    if (result.type() != TestResultType::Success) {
        if (!test.description().empty()) {
            const auto nextLines = sfun::after(test.description(), "\n");
            auto descriptionHasMultipleLines = nextLines.has_value() && !nextLines.value().empty();
            if (descriptionHasMultipleLines)
                print("Description:\n{}", test.description());
            else
                print("Description: {}", test.description());
        }
        if (!result.failedActionsMessages().empty()) {
            if (std::ssize(result.failedActionsMessages()) > 1)
                print("Failure:\n{}", sfun::join(result.failedActionsMessages(), "\n"));
            else
                print("Failure: {}", sfun::join(result.failedActionsMessages(), "\n"));
        }
    }
    if (result.type() == TestResultType::RuntimeError)
        if (!result.errorInfo().empty())
            lunchtoast::print(fmt::runtime(result.errorInfo()));
    const auto resultStr = fmt::format("Result: {:>10}", testResultStr(result.type()));
    print(result.type(), fmt::runtime("{:>" + std::to_string(reportWidth_) + "}"), resultStr);
}

void TestReporter::reportBrokenTest(
        const fs::path& brokenTestConfig,
        const std::string& errorInfo,
        std::string suiteName,
        int suiteTestNumber,
        std::ptrdiff_t suiteNumOfTests) const
{
    suiteName = truncateString(suiteName, reportWidth_ / 2);
    auto header = fmt::format(" {} [ {} / {} ] ", suiteName, suiteTestNumber, suiteNumOfTests);
    if (suiteName.empty())
        header = header.substr(1);

    print(TestResultType::Failure, fmt::runtime("{:#^" + std::to_string(reportWidth_) + "}"), header);
    print("Test can't be started. Config file {} error:\n{}\n", homePathString(brokenTestConfig), errorInfo);
}

void TestReporter::reportDisabledTest(
        const Test& test,
        std::string suiteName,
        int suiteTestNumber,
        sfun::ssize_t suiteNumOfTests) const
{
    suiteName = truncateString(suiteName, reportWidth_ / 2);
    auto header = fmt::format(" {} [ {} / {} ] ", suiteName, suiteTestNumber, suiteNumOfTests);
    if (suiteName.empty())
        header = header.substr(1);
    lunchtoast::print(fmt::runtime("{:#^" + std::to_string(reportWidth_) + "}"), header);
    print("Name: {}", test.name());

    if (!test.description().empty()) {
        const auto nextLines = sfun::after(test.description(), "\n");
        const auto descriptionHasMultipleLines = nextLines.has_value() && !nextLines.value().empty();
        if (descriptionHasMultipleLines)
            print("Description:\n{}", test.description());
        else
            print("Description: {}", test.description());
    }

    const auto resultStr = fmt::format("Result: {:>10}", "DISABLED");
    lunchtoast::print(fmt::runtime("{:>" + std::to_string(reportWidth_) + "}"), resultStr);
}

namespace {
std::tuple<int, int, int> countTotals(const TestSuite& defaultSuite, const std::map<std::string, TestSuite>& suites)
{
    auto totalTests = std::ssize(defaultSuite.tests);
    auto totalPassed = defaultSuite.passedTestsCounter;
    auto totalDisabled = defaultSuite.disabledTestsCounter;
    for (const auto& suite : suites | views::values) {
        totalTests += std::ssize(suite.tests);
        totalPassed += suite.passedTestsCounter;
        totalDisabled += suite.disabledTestsCounter;
    }
    return std::make_tuple(static_cast<int>(totalTests), totalPassed, totalDisabled);
}

void reportSuiteResult(
        std::string suiteName,
        int passedNumber,
        sfun::ssize_t totalNumber,
        int disabledNumber,
        int reportWidth)
{
    if (totalNumber == 0 && disabledNumber == 0)
        return;
    const auto failedNumber = totalNumber - disabledNumber - passedNumber;
    const auto width = reportWidth / 2 + 4;
    suiteName = truncateString(suiteName, width - 1) + ":";
    const auto resultType = failedNumber ? TestResultType::Failure : TestResultType::Success;
    auto resultStr = std::string{};
    if (disabledNumber)
        resultStr = fmt::format(
                "{} out of {} passed, {} failed, {} disabled",
                passedNumber,
                totalNumber,
                failedNumber,
                disabledNumber);
    else
        resultStr = fmt::format("{} out of {} passed, {} failed", passedNumber, totalNumber, failedNumber);
    print(resultType, fmt::runtime("{:" + std::to_string(width) + "} {}"), suiteName, resultStr);
}

} //namespace

void TestReporter::reportSummary(const TestSuite& defaultSuite, const std::map<std::string, TestSuite>& suites) const
{
    auto [totalTests, totalPassed, totalDisabled] = countTotals(defaultSuite, suites);
    if (totalTests == 0 && totalDisabled == 0) {
        print("No tests were found. Exiting.");
        return;
    }

    if (totalTests)
        printNewLine();
    lunchtoast::print(fmt::runtime("{:#^" + std::to_string(reportWidth_) + "}"), "  SUMMARY  ");
    reportSuiteResult(
            "Default",
            defaultSuite.passedTestsCounter,
            std::ssize(defaultSuite.tests),
            defaultSuite.disabledTestsCounter,
            reportWidth_);
    for (const auto& [suiteName, suite] : suites) {
        reportSuiteResult(
                suiteName,
                suite.passedTestsCounter,
                std::ssize(suite.tests),
                suite.disabledTestsCounter,
                reportWidth_);
    }
    print("---");
    reportSuiteResult("Total", totalPassed, totalTests, totalDisabled, reportWidth_);
}

} //namespace lunchtoast