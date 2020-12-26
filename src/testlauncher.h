#pragma once
#include "alias_boost_filesystem.h"
#include <map>

class Test;
class TestResult;

struct TestSuite{
    std::vector<fs::path> tests;
    int passedTestsCounter = 0;
};

class TestLauncher
{
public:
    TestLauncher(const fs::path& testPath,
                 const std::string& testFileExt = ".toast",
                 const fs::path& reportFilePath = {},
                 const int reportWidth = 48);

    bool process();

private:
    void collectTests(const fs::path& testPath);
    void addTest(const fs::path& testFile);
    bool launchTest(const fs::path& testPath);
    void reportResult(const Test& test, const TestResult& result,
                      const std::string& suite, int suiteTestNumber, int suiteNumOfTests);
    void reportBrokenTest(const fs::path& brokenTestConfig, const std::string& errorInfo,
                          const std::string& suite, int suiteTestNumber, int suiteNumOfTests);
    void reportSuiteResult(std::string suiteName, int passedNumber, int totalNumber);
    void reportSummary();
    void initReporter(const fs::path& reportFilePath);
    bool processSuite(const std::string& suiteName, TestSuite& suite);


private:
    fs::path testPath_;
    std::string testFileExt_;
    std::map<std::string, TestSuite> suites_;
    TestSuite defaultSuite_;
    const int reportWidth_;
};

