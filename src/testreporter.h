#pragma once
#include "testsuite.h"
#include <sfun/utility.h>
#include <filesystem>
#include <map>
#include <string>

namespace lunchtoast {

class Test;
class TestResult;

class TestReporter {
public:
    TestReporter(const std::filesystem::path& reportFilePath, int reportWidth);
    void reportResult(
            const Test& test,
            const TestResult& result,
            std::string suiteName,
            int suiteTestNumber,
            sfun::ssize_t suiteNumOfTests) const;
    void reportBrokenTest(
            const std::filesystem::path& brokenTestConfig,
            const std::string& errorInfo,
            std::string suiteName,
            int suiteTestNumber,
            sfun::ssize_t suiteNumOfTests) const;
    void reportDisabledTest( //
            const Test& test,
            std::string suiteName,
            int suiteTestNumber,
            sfun::ssize_t suiteNumOfTests) const;
    void reportSummary(const TestSuite& defaultSuite, const std::map<std::string, TestSuite>& suites) const;

private:
    int reportWidth_;
};

} //namespace lunchtoast
