#pragma once
#include "testsuite.h"
#include <filesystem>
#include <map>


namespace lunchtoast{

class Test;
class TestResult;
class TestReporter;

class TestLauncher{
public:
    TestLauncher(const std::filesystem::path& testPath,
                 const std::string& testFileExt,
                 const TestReporter& reporter);
    bool process();

private:
    void collectTests(const std::filesystem::path& testPath, const std::string& testFileExt);
    void addTest(const std::filesystem::path& testFile);
    bool processSuite(const std::string& suiteName, TestSuite& suite);

private:
    TestSuite defaultSuite_;
    std::map<std::string, TestSuite> suites_;
    const TestReporter& reporter_;
};

}