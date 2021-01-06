#pragma once
#include "alias_boost_filesystem.h"
#include "testsuite.h"
#include <string>
#include <map>

class Test;
class TestResult;

class TestReporter
{
public:
    TestReporter(const fs::path& reportFilePath,
                 const int reportWidth);
    void reportResult(const Test& test, const TestResult& result,
                      std::string suiteName, int suiteTestNumber, int suiteNumOfTests) const;
    void reportBrokenTest(const fs::path& brokenTestConfig, const std::string& errorInfo,
                          std::string suiteName, int suiteTestNumber, int suiteNumOfTests) const;
    void reportSummary(const TestSuite& defaultSuite, const std::map<std::string, TestSuite>& suites) const;

private:
    void initReporter(const fs::path& reportFilePath);
    void reportSuiteResult(std::string suiteName, int passedNumber, int totalNumber, int disabledNumber) const;

private:
    const int reportWidth_;
};

