#pragma once
#include "testsuite.h"
#include "useraction.h"
#include <sfun/member.h>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
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
            std::vector<std::filesystem::path> configList,
            std::optional<int> searchDirectoryLevels);
    void addTest(const std::filesystem::path& testFile, const std::vector<std::filesystem::path>& configList);
    std::vector<std::filesystem::path> processSuite(const std::string& suiteName, TestSuite& suite);
    const TestReporter& reporter() const;

private:
    TestSuite defaultSuite_;
    std::map<std::string, TestSuite> suites_;
    sfun::member<const TestReporter&> reporter_;
    sfun::member<const Config&> config_;
    sfun::member<const std::vector<UserAction>> userActions_;
    sfun::member<const std::string> shellCommand_;
    sfun::member<const bool> cleanup_;
    sfun::member<const std::vector<std::string>> selectedTags_;
    sfun::member<const std::vector<std::string>> skippedTags_;
    sfun::member<const std::filesystem::path> listOfFailedTests_;
    sfun::member<const std::filesystem::path> dirWithFailedTests_;
};

} //namespace lunchtoast