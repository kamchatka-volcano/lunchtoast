#pragma once
#include "testsuite.h"
#include <filesystem>
#include <functional>
#include <map>

namespace lunchtoast {

class Test;
class TestResult;
class TestReporter;

class TestLauncher {
public:
    TestLauncher(
            const std::filesystem::path& testPath,
            const std::string& testFileExt,
            std::string shellCommand,
            bool cleanup,
            const TestReporter& reporter,
            std::vector<std::string> selectedTags,
            std::vector<std::string> skippedTags);
    bool process();

private:
    void collectTests(const std::filesystem::path& testPath, const std::string& testFileExt);
    void addTest(const std::filesystem::path& testFile);
    bool processSuite(const std::string& suiteName, TestSuite& suite);
    const TestReporter& reporter();

private:
    TestSuite defaultSuite_;
    std::map<std::string, TestSuite> suites_;
    std::reference_wrapper<const TestReporter> reporter_;
    std::string shellCommand_;
    bool cleanup_;
    std::vector<std::string> selectedTags_;
    std::vector<std::string> skippedTags_;
};

} //namespace lunchtoast