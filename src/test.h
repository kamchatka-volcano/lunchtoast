#pragma once
#include "filenamereader.h"
#include "launchprocessresult.h"
#include "section.h"
#include "testaction.h"
#include "testresult.h"
#include "useraction.h"
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace lunchtoast {

class Test {
public:
    explicit Test(
            const std::filesystem::path& testCasePath,
            const std::unordered_map<std::string, std::string>& vars,
            std::vector<UserAction> userActions,
            std::string shellCommand,
            bool cleanup);
    TestResult process();

    const std::string& suite() const;
    const std::string& name() const;
    const std::string& description() const;

private:
    void readTestCase(const std::filesystem::path& path, const std::unordered_map<std::string, std::string>& vars);
    std::vector<Section> readParam(const std::vector<Section>& sections);
    std::vector<Section> readAction(
            const std::vector<Section>& sections,
            const std::unordered_map<std::string, std::string>& vars);
    std::vector<Section> createLaunchAction(const Section& section, const std::vector<Section>& nextSections);
    void createWriteAction(const Section& section);
    void createCompareFilesAction(
            TestActionType actionType,
            const std::string& comparisonType,
            const std::string& filenamesStr);
    void createCompareFileContentAction(
            TestActionType actionType,
            const std::string& filenameStr,
            const std::string& expectedFileContent);
    void createComparisonAction(
            TestActionType actionType,
            const std::string& encodedActionType,
            const Section& section);
    void cleanTestFiles();
    bool readParam(std::string& param, const std::string& paramName, const Section& section);
    bool readParam(std::filesystem::path& param, const std::string& paramName, const Section& section);
    bool readParam(std::vector<FilenameGroup>& param, const std::string& paramName, const Section& section);
    bool readParam(bool& param, const std::string& paramName, const Section& section);
    void postProcessCleanupConfig(const std::filesystem::path& testCasePath);
    void checkParams();

private:
    std::vector<TestAction> actions_;
    std::vector<UserAction> userActions_;
    std::string name_;
    std::string description_;
    std::filesystem::path directory_;
    std::string suite_;
    std::string shellCommand_;
    bool isEnabled_;
    bool cleanup_;
    std::vector<FilenameGroup> contents_;
    std::optional<LaunchProcessResult> launchActionResult_;
};

} //namespace lunchtoast