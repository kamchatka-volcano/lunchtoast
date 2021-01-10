#include "test.h"
#include "sectionsreader.h"
#include "launchprocess.h"
#include "writefile.h"
#include "comparefiles.h"
#include "comparefilecontent.h"
#include "utils.h"
#include "string_utils.h"
#include <spdlog/fmt/fmt.h>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <iomanip>

Test::Test(const fs::path& configPath)
    : name_(configPath.stem().string())
    , directory_(configPath.parent_path())
    , shellCommand_("sh -c -e")
    , isEnabled_(true)
    , requiresCleanup_(false)
{
    readConfig(configPath);
    postProcessCleanupConfig(configPath);
}

TestConfigError::TestConfigError(const std::string& msg)
    : std::runtime_error(msg)
{
}

TestResult Test::process()
{
    if (requiresCleanup_)
        cleanTestFiles();

    auto failedActionsMessages = std::vector<std::string>{};
    if (actions_.empty())
        return TestResult::RuntimeError("Test has nothing to check", failedActionsMessages);

    auto ok = true;
    for (const auto& action : actions_){
        try{
            auto result = action.process();
            if(!result.isSuccessful()){
                ok = false;
                failedActionsMessages.push_back(result.errorInfo());
            }
        } catch(const std::exception& e){
            return TestResult::RuntimeError(e.what(), failedActionsMessages);
        }        
        auto stopOnFailure = (action.type() == TestActionType::Assertion ||
                              action.type() == TestActionType::RequiredOperation);
        if (!ok && stopOnFailure)
            break;
    }

    if (ok && requiresCleanup_)
        cleanTestFiles();

    if (ok)
        return TestResult::Success();
    else
        return TestResult::Failure(failedActionsMessages);
}

bool Test::readParamFromSection(const Section& section)
{
    if (readParam(name_, "Name", section)) return true;
    if (readParam(suite_, "Suite", section)) return true;
    if (readParam(description_, "Description", section)) return true;
    if (readParam(directory_, "Directory", section)) return true;
    if (readParam(isEnabled_, "Enabled", section)) return true;
    if (readParam(cleanupWhitelist_, "Cleanup whitelist", section)) return true;
    if (readParam(requiresCleanup_, "Cleanup", section)) return true;
    if (readParam(shellCommand_, "Shell", section)) return true;
    return false;
}

bool Test::readActionFromSection(const Section &section)
{
    if (boost::starts_with(section.name, "Launch")){
        createLaunchAction(section);
        return true;
    }
    if (boost::starts_with(section.name, "Write")){
        createWriteAction(section);
        return true;
    }
    if (boost::starts_with(section.name, "Assert")){
        auto actionType = boost::trim_copy(str::after(section.name, "Assert"));
        return createComparisonAction(TestActionType::Assertion, actionType, section.value);
    }
    if (boost::starts_with(section.name, "Expect")){
        auto actionType = boost::trim_copy(str::after(section.name, "Expect"));
        return createComparisonAction(TestActionType::Expectation, actionType, section.value);
    }
    return false;
}

void Test::readConfig(const boost::filesystem::path& path)
{
    auto fileStream = std::ifstream{path.string()};
    if (!fileStream.is_open())
        throw TestConfigError{fmt::format("Test config file {} doesn't exist", path.string())};

    try{
        const auto sections = readSections(fileStream, {RawSectionSpecifier{"Write", "---"},
                                                        RawSectionSpecifier{"Assert content of", "---"},
                                                        RawSectionSpecifier{"Expect content of", "---"}});
        if (sections.empty())
            throw TestConfigError{fmt::format("Test config file {} is empty or invalid", path.string())};
        for (const auto& section : sections){
            if (readParamFromSection(section))
                continue;
            if (readActionFromSection(section))
                continue;
            throw TestConfigError{fmt::format("Unsupported section name: {}", section.name)};
        }
    } catch(const std::exception& e){
        throw TestConfigError{e.what()};
    }

    checkParams();
    processVariablesSubstitution(name_, path.stem().string(), directory_.stem().string());
    processVariablesSubstitution(description_, path.stem().string(), directory_.stem().string());
    processVariablesSubstitution(suite_, path.stem().string(), directory_.stem().string());
}

void Test::checkParams()
{
    if (!fs::exists(directory_))
        throw TestConfigError{fmt::format("Specified directory '{}' doesn't exist", directory_.string())};
}

bool Test::createComparisonAction(TestActionType type, const std::string& encodedActionType, const std::string& value)
{
    if (encodedActionType == "files equal"){
        createCompareFilesAction(type, value);
        return true;
    }
    if (boost::starts_with(encodedActionType, "content of ")){
        createCompareFileContentAction(type, encodedActionType, value);
        return true;
    }
    return false;
}

void Test::createLaunchAction(const Section& section)
{
    const auto parts = str::splitted(section.name);
    auto uncheckedResult = std::find(parts.begin(), parts.end(), "unchecked") != parts.end();
    auto isShellCommand = std::find(parts.begin(), parts.end(), "command") != parts.end();
    auto silently = std::find(parts.begin(), parts.end(), "silently") != parts.end();
    const auto command = boost::trim_copy(section.value);
    actions_.push_back(LaunchProcess{command, directory_, (isShellCommand ? shellCommand_ : ""), uncheckedResult, silently});
}

void Test::createWriteAction(const Section& section)
{
    const auto fileName = boost::trim_copy(str::after(section.name, "Write"));
    const auto path = fs::absolute(fileName, directory_);
    actions_.push_back(WriteFile{path.string(), section.value});
}

void Test::createCompareFilesAction(TestActionType type, const std::string& filenamesStr)
{
    const auto filenameGroups = readFilenames(filenamesStr, directory_);
    if (filenameGroups.size() != 2)
        throw TestConfigError{"Comparison of files require exactly two filenames or filename matching regular expressions to be specified"};    
    actions_.push_back(TestAction{CompareFiles{filenameGroups[0], filenameGroups[1]}, type});
}

void Test::createCompareFileContentAction(TestActionType type, const std::string& filenameStr, const std::string &expectedFileContent)
{
    const auto filename = boost::trim_copy(boost::replace_first_copy(filenameStr, "content of ", ""));
    actions_.push_back(TestAction{CompareFileContent{fs::absolute(filename, directory_), expectedFileContent}, type});
}

void Test::cleanTestFiles()
{
    if (cleanupWhitelist_.empty())
        return;
    auto whiteListPaths = std::set<fs::path>{};
    for (const auto& filenameGroup : cleanupWhitelist_){
        const auto paths = filenameGroup.pathList();
        std::copy(paths.begin(), paths.end(), std::inserter(whiteListPaths, whiteListPaths.end()));
    }

    auto paths = getDirectoryContent(directory_);
    std::sort(paths.begin(), paths.end(), std::greater<>{});

    for (const auto& path : paths)
        if (!whiteListPaths.count(path))
            fs::remove(path);
}

const std::string& Test::suite() const
{
    return suite_;
}

const std::string& Test::name() const
{
    return name_;
}

const std::string& Test::description() const
{
    return description_;
}

bool Test::readParam(std::string& param, const std::string& paramName, const Section& section)
{
    if (section.name != paramName)
        return false;
    param = boost::trim_copy(section.value);    
    return true;
}

bool Test::readParam(fs::path& param, const std::string& paramName, const Section& section)
{
    if (section.name != paramName)
        return false;
    param = fs::absolute(boost::trim_copy(section.value), directory_);
    return true;
}

bool Test::readParam(std::vector<FilenameGroup>& param, const std::string& paramName, const Section& section)
{
    if (section.name != paramName)
        return false;
    param = readFilenames(section.value, directory_);
    return true;
}

bool Test::readParam(bool& param, const std::string& paramName, const Section& section)
{
    if (section.name != paramName)
        return false;
    auto paramStr = boost::to_lower_copy(boost::trim_copy(section.value));
    param = (paramStr == "true");
    return true;
}

void Test::postProcessCleanupConfig(const fs::path& configPath)
{
    if (cleanupWhitelist_.empty()){
        auto pathList = getDirectoryContent(directory_);
        std::transform(pathList.begin(), pathList.end(), std::back_inserter(cleanupWhitelist_), [this](const fs::path& path){
            return FilenameGroup{fs::relative(path, directory_).string(), directory_};
        });
        return;
    }
    cleanupWhitelist_.push_back(FilenameGroup{fs::relative(configPath, directory_).string(), directory_});
}
