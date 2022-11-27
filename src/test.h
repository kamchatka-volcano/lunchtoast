#pragma once
#include "section.h"
#include "testresult.h"
#include "testaction.h"
#include "filenamereader.h"
#include <filesystem>
#include <vector>
#include <memory>
#include <set>
#include <string>


namespace lunchtoast{

class Test{
public:
    explicit Test(const std::filesystem::path& configPath);
    TestResult process();

    const std::string& suite() const;
    const std::string& name() const;
    const std::string& description() const;

private:
    void readConfig(const std::filesystem::path& path);
    bool readParamFromSection(const Section& section);
    bool readActionFromSection(const Section& section);
    void createLaunchAction(const Section& section);
    void createWriteAction(const Section& section);
    void createCompareFilesAction(TestActionType type, const std::string& filenamesStr);
    void createCompareFileContentAction(TestActionType type, const std::string& filenameStr,
                                        const std::string& expectedFileContent);
    bool createComparisonAction(TestActionType type, const std::string& encodedActionType, const Section& section);
    void cleanTestFiles();
    bool readParam(std::string& param, const std::string& paramName, const Section& section);
    bool readParam(std::filesystem::path& param, const std::string& paramName, const Section& section);
    bool readParam(std::vector<FilenameGroup>& param, const std::string& paramName, const Section& section);
    bool readParam(bool& param, const std::string& paramName, const Section& section);
    void postProcessCleanupConfig(const std::filesystem::path& configPath);
    void checkParams();

private:
    std::vector<std::unique_ptr<TestAction>> actions_;
    std::string name_;
    std::string description_;
    std::filesystem::path directory_;
    std::string suite_;
    std::string shellCommand_;
    bool isEnabled_;
    bool requiresCleanup_;
    std::vector<FilenameGroup> cleanupWhitelist_;
};

}