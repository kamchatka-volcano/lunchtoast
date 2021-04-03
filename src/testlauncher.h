#pragma once
#include "testsuite.h"
#include "alias_filesystem.h"
#include <map>

class Test;
class TestResult;
class TestReporter;

class TestLauncher
{
public:
    TestLauncher(const fs::path& testPath,
                 const std::string& testFileExt,
                 const TestReporter& reporter);
    bool process();

private:
    void collectTests(const fs::path& testPath, const std::string& testFileExt);
    void addTest(const fs::path& testFile);
    bool processSuite(const std::string& suiteName, TestSuite& suite);

private:
    TestSuite defaultSuite_;
    std::map<std::string, TestSuite> suites_;
    const TestReporter& reporter_;
};

