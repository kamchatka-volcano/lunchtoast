#pragma once
#include "testresult.h"
#include "testaction.h"
#include "filenamereader.h"
#include "alias_filesystem.h"
#include <vector>
#include <memory>
#include <set>
#include <string>

struct Section;

class Test
{
public:
    explicit Test(const fs::path& configPath);
    TestResult process();

    const std::string& suite() const;
    const std::string& name() const;
    const std::string& description() const;

private:
    void readConfig(const fs::path& path);
    bool readParamFromSection(const Section& section);
    bool readActionFromSection(const Section& section);
    void createLaunchAction(const Section& section);
    void createWriteAction(const Section& section);
    void createCompareFilesAction(TestActionType type, const std::string& filenamesStr);
    void createCompareFileContentAction(TestActionType type, const std::string& filenameStr, const std::string& expectedFileContent);
    bool createComparisonAction(TestActionType type, const std::string& encodedActionType, const std::string& value);
    void cleanTestFiles();
    bool readParam(std::string& param, const std::string& paramName, const Section& section);
    bool readParam(fs::path& param, const std::string& paramName, const Section& section);
    bool readParam(std::vector<FilenameGroup>& param, const std::string& paramName, const Section& section);
    bool readParam(bool& param, const std::string& paramName, const Section& section);
    void postProcessCleanupConfig(const fs::path& configPath);
    void checkParams();

private:
    std::vector<std::unique_ptr<TestAction>> actions_;
    std::string name_;
    std::string description_;
    fs::path directory_;
    std::string suite_;
    std::string shellCommand_;
    bool isEnabled_;
    bool requiresCleanup_;
    std::vector<FilenameGroup> cleanupWhitelist_;
};

class TestConfigError: public std::runtime_error{
public:
    explicit TestConfigError(const std::string& msg);
};
