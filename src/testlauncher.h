#pragma once
#include "testsuite.h"
#include <filesystem>
#include <functional>
#include <map>
#include <vector>

namespace lunchtoast {

class Test;
class TestResult;
class TestReporter;
struct CommandLine;

class TestLauncher {
public:
    TestLauncher(const TestReporter&, const CommandLine&);
    bool process();

private:
    void collectTests(const std::filesystem::path& testPath, const std::string& testFileExt);
    void addTest(const std::filesystem::path& testFile);
    std::vector<std::filesystem::path> processSuite(const std::string& suiteName, TestSuite& suite);
    const TestReporter& reporter();

private:
    TestSuite defaultSuite_;
    std::map<std::string, TestSuite> suites_;
    std::reference_wrapper<const TestReporter> reporter_;
    std::string shellCommand_;
    bool cleanup_;
    std::vector<std::string> selectedTags_;
    std::vector<std::string> skippedTags_;
    std::filesystem::path listOfFailedTests_;
    std::filesystem::path dirWithFailedTests_;
};

} //namespace lunchtoast