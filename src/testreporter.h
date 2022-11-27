#pragma once
#include "testsuite.h"
#include <filesystem>
#include <string>
#include <map>


namespace lunchtoast{

class Test;
class TestResult;

class TestReporter{
public:
    TestReporter(const std::filesystem::path& reportFilePath,
                 int reportWidth);
    void reportResult(const Test& test, const TestResult& result,
                      std::string suiteName, int suiteTestNumber, int suiteNumOfTests) const;
    void reportBrokenTest(const std::filesystem::path& brokenTestConfig, const std::string& errorInfo,
                          std::string suiteName, int suiteTestNumber, int suiteNumOfTests) const;
    void reportDisabledTest(const Test& test,
                            std::string suiteName, int suiteTestNumber, int suiteNumOfTests) const;
    void reportSummary(const TestSuite& defaultSuite, const std::map<std::string, TestSuite>& suites) const;

private:
    void initReporter(const std::filesystem::path& reportFilePath);
    void reportSuiteResult(std::string suiteName, int passedNumber, int totalNumber, int disabledNumber) const;

private:
    const int reportWidth_;
};

}
