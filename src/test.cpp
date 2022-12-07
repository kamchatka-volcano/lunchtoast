#include "test.h"
#include "sectionsreader.h"
#include "launchprocess.h"
#include "writefile.h"
#include "comparefiles.h"
#include "comparefilecontent.h"
#include "utils.h"
#include "errors.h"
#include <sfun/string_utils.h>
#include <fmt/format.h>
#include <sstream>
#include <iomanip>
#include <fstream>


namespace lunchtoast{
namespace fs = std::filesystem;

Test::Test(const fs::path& configPath, std::string shellCommand, bool cleanup)
    : name_(configPath.stem().string())
    , directory_(configPath.parent_path())
    , shellCommand_(std::move(shellCommand))
    , isEnabled_(true)
    , cleanup_(cleanup)
{
    readConfig(configPath);
    postProcessCleanupConfig(configPath);
}

TestResult Test::process()
{
    if (cleanup_)
        cleanTestFiles();

    auto failedActionsMessages = std::vector<std::string>{};
    if (actions_.empty())
        return TestResult::RuntimeError("Test has nothing to check", failedActionsMessages);

    auto ok = true;
    for (auto& action: actions_){
        try{
            auto result = action->process();
            if (!result.isSuccessful()){
                ok = false;
                failedActionsMessages.push_back(result.errorInfo());
            }
        } catch (const std::exception& e){
            return TestResult::RuntimeError(e.what(), failedActionsMessages);
        }
        auto stopOnFailure = (action->type() == TestActionType::Assertion ||
                              action->type() == TestActionType::RequiredOperation);
        if (!ok && stopOnFailure)
            break;
    }

    if (ok && cleanup_)
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
    return false;
}

bool Test::readActionFromSection(const Section& section)
{
    if (sfun::startsWith(section.name, "Launch")){
        createLaunchAction(section);
        return true;
    }
    if (sfun::startsWith(section.name, "Write")){
        createWriteAction(section);
        return true;
    }
    if (sfun::startsWith(section.name, "Assert")){
        auto actionType = sfun::trim(sfun::after(section.name, "Assert"));
        return createComparisonAction(TestActionType::Assertion, std::string{actionType}, section);
    }
    if (sfun::startsWith(section.name, "Expect")){
        auto actionType = sfun::trim(sfun::after(section.name, "Expect"));
        return createComparisonAction(TestActionType::Expectation, std::string{actionType}, section);
    }
    return false;
}

namespace{
bool isValidUnusedSection(const Section& section)
{
    if (section.name == "Section separator")
        return true;

    return false;
}
}

void Test::readConfig(const fs::path& path)
{
    auto fileStream = std::ifstream{path.string()};
    if (!fileStream.is_open())
        throw TestConfigError{fmt::format("Test config file {} doesn't exist", homePathString(path))};

    try{
        const auto sections = readSections(fileStream);
        if (sections.empty())
            throw TestConfigError{fmt::format("Test config file {} is empty or invalid", homePathString(path))};
        for (const auto& section: sections){
            if (readParamFromSection(section))
                continue;
            if (readActionFromSection(section))
                continue;
            if (isValidUnusedSection(section))
                continue;
            throw TestConfigError{fmt::format("Unsupported section name: {}", section.name)};
        }
    } catch (const std::exception& e){
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
        throw TestConfigError{fmt::format("Specified directory '{}' doesn't exist", homePathString(directory_))};
}

bool Test::createComparisonAction(TestActionType type, const std::string& encodedActionType, const Section& section)
{
    if (encodedActionType == "files equal"){
        createCompareFilesAction(type, section.value);
        return true;
    }
    if (sfun::startsWith(encodedActionType, "content of ")){
        createCompareFileContentAction(type, encodedActionType, section.value);
        return true;
    }
    return false;
}

void Test::createLaunchAction(const Section& section)
{
    const auto parts = sfun::split(section.name);
    auto uncheckedResult = std::find(parts.begin(), parts.end(), "unchecked") != parts.end();
    auto isShellCommand = std::find(parts.begin(), parts.end(), "command") != parts.end();
    auto silently = std::find(parts.begin(), parts.end(), "silently") != parts.end();
    const auto& command = section.value;
    actions_.push_back(
            std::make_unique<LaunchProcess>(command, directory_, (isShellCommand ? shellCommand_ : ""), uncheckedResult,
                                            silently));
}

void Test::createWriteAction(const Section& section)
{
    const auto fileName = sfun::trim(sfun::after(section.name, "Write"));
    const auto path = fs::absolute(directory_) / fileName;
    actions_.push_back(std::make_unique<WriteFile>(path.string(), section.value));
}

void Test::createCompareFilesAction(TestActionType type, const std::string& filenamesStr)
{
    const auto filenameGroups = readFilenames(filenamesStr, directory_);
    if (filenameGroups.size() != 2)
        throw TestConfigError{
                "Comparison of files require exactly two filenames or filename matching regular expressions to be specified"};
    actions_.push_back(std::make_unique<CompareFiles>(filenameGroups[0], filenameGroups[1], type));
}

void Test::createCompareFileContentAction(TestActionType type, const std::string& filenameStr,
                                          const std::string& expectedFileContent)
{
    const auto filename = std::string{sfun::trim(sfun::replace(filenameStr, "content of ", ""))};
    actions_.push_back(
            std::make_unique<CompareFileContent>(fs::absolute(directory_) / filename, expectedFileContent, type));
}

void Test::cleanTestFiles()
{
    if (cleanupWhitelist_.empty())
        return;
    auto whiteListPaths = std::set<fs::path>{};
    for (const auto& filenameGroup: cleanupWhitelist_){
        const auto paths = filenameGroup.pathList();
        std::copy(paths.begin(), paths.end(), std::inserter(whiteListPaths, whiteListPaths.end()));
    }

    auto paths = getDirectoryContent(directory_);
    std::sort(paths.begin(), paths.end(), std::greater<>{});

    for (const auto& path: paths)
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
    param = section.value;
    return true;
}

bool Test::readParam(fs::path& param, const std::string& paramName, const Section& section)
{
    if (section.name != paramName)
        return false;
    param = fs::absolute(directory_) / sfun::trim(section.value);
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
    auto paramStr = toLower(sfun::trim(section.value));
    param = (paramStr == "true");
    return true;
}

void Test::postProcessCleanupConfig(const fs::path& configPath)
{
    if (cleanupWhitelist_.empty()){
        auto pathList = getDirectoryContent(directory_);
        std::transform(pathList.begin(), pathList.end(), std::back_inserter(cleanupWhitelist_),
                       [this](const fs::path& path){
                           return FilenameGroup{fs::relative(path, directory_).string(), directory_};
                       });
        return;
    }
    cleanupWhitelist_.emplace_back(fs::relative(configPath, directory_).string(), directory_);
}

}