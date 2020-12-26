#pragma once
#include "testresult.h"
#include "testaction.h"
#include "alias_boost_filesystem.h"
#include <vector>
#include <set>
#include <string>

class Section;

class Test
{
public:
    Test(const fs::path& configPath);
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
    void saveDirectoryState();
    void restoreDirectoryState();

private:
    std::vector<TestAction> actions_;
    std::string name_;
    std::string description_;
    fs::path directory_;
    std::string suite_;
    std::string shellCommand_;
    bool isEnabled_;
    bool requiresCleanup_;
    std::set<fs::path> directoryState_;

};

class TestConfigError: public std::runtime_error{
public:
    TestConfigError(const std::string& msg);
};
