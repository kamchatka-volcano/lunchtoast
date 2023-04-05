#pragma once
#include "testsuite.h"
#include "useraction.h"
#include <filesystem>
#include <functional>
#include <map>
#include <vector>

namespace lunchtoast {

class Test;
class TestResult;
class TestReporter;
struct CommandLine;
struct Config;

class TestLauncher {
public:
    TestLauncher(const TestReporter&, const CommandLine&, const Config&);
    bool process();

private:
    void collectTests(
            const std::filesystem::path& testPath,
            const std::string& testFileExt,
            std::vector<std::filesystem::path> configList);
    void addTest(const std::filesystem::path& testFile, const std::vector<std::filesystem::path>& configList);
    std::vector<std::filesystem::path> processSuite(const std::string& suiteName, TestSuite& suite);
    const TestReporter& reporter();

private:
    TestSuite defaultSuite_;
    std::map<std::string, TestSuite> suites_;
    std::reference_wrapper<const TestReporter> reporter_;
    std::reference_wrapper<const Config> config_;
    std::vector<UserAction> userActions_;
    std::string shellCommand_;
    bool cleanup_;
    std::vector<std::string> selectedTags_;
    std::vector<std::string> skippedTags_;
    std::filesystem::path listOfFailedTests_;
    std::filesystem::path dirWithFailedTests_;
};

} //namespace lunchtoast