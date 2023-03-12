#include "test.h"
#include "compareerroroutput.h"
#include "compareexitcode.h"
#include "comparefilecontent.h"
#include "comparefiles.h"
#include "compareoutput.h"
#include "errors.h"
#include "launchprocess.h"
#include "sectionsreader.h"
#include "utils.h"
#include "writefile.h"
#include <fmt/format.h>
#include <sfun/functional.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace lunchtoast {
namespace fs = std::filesystem;

Test::Test(
        const fs::path& configPath,
        const std::unordered_map<std::string, std::string>& vars,
        std::string shellCommand,
        bool cleanup)
    : name_(sfun::pathString(configPath.stem()))
    , directory_(configPath.parent_path())
    , shellCommand_(std::move(shellCommand))
    , isEnabled_(true)
    , cleanup_(cleanup)
{
    readConfig(configPath, vars);
    postProcessCleanupConfig(configPath);
}

TestResult Test::process()
{
    if (cleanup_)
        cleanTestFiles();

    auto failedActionsMessages = std::vector<std::string>{};
    if (actions_.empty())
        return TestResult::RuntimeError("Test has nothing to check", failedActionsMessages);

    auto onActionSuccessful = sfun::overloaded{
            [](auto&) {},
            [&](LaunchProcess& launchAction)
            {
                launchActionResult_ = launchAction.result();
            }};

    auto ok = true;
    auto onActionFailed = [&](auto&, const std::string& errorInfo)
    {
        ok = false;
        failedActionsMessages.push_back(errorInfo);
    };

    auto runtimeError = std::optional<std::string>{};
    auto onActionError = [&](auto&, const std::string& errorInfo)
    {
        runtimeError = errorInfo;
    };

    for (auto actionIt = actions_.begin(); actionIt != actions_.end(); ++actionIt) {
        nextAction_ = [&]() -> std::optional<TestAction>
        {
            auto nextActionIt = std::next(actionIt);
            if (nextActionIt == actions_.end())
                return std::nullopt;
            return *nextActionIt;
        }();

        actionIt->process(onActionSuccessful, onActionFailed, onActionError);

        if (runtimeError)
            return TestResult::RuntimeError(runtimeError.value(), failedActionsMessages);

        auto stopOnFailure =
                (actionIt->type() == TestActionType::Assertion ||
                 actionIt->type() == TestActionType::RequiredOperation);

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
    if (readParam(name_, "Name", section))
        return true;
    if (readParam(suite_, "Suite", section))
        return true;
    if (readParam(description_, "Description", section))
        return true;
    if (readParam(directory_, "Directory", section))
        return true;
    if (readParam(isEnabled_, "Enabled", section))
        return true;
    if (readParam(contents_, "Contents", section))
        return true;
    return false;
}

bool Test::readActionFromSection(const Section& section)
{
    if (sfun::startsWith(section.name, "Launch")) {
        createLaunchAction(section);
        return true;
    }
    if (sfun::startsWith(section.name, "Write")) {
        createWriteAction(section);
        return true;
    }
    if (sfun::startsWith(section.name, "Assert")) {
        auto actionType = sfun::trim(sfun::after(section.name, "Assert"));
        return createComparisonAction(TestActionType::Assertion, std::string{actionType}, section);
    }
    if (sfun::startsWith(section.name, "Expect")) {
        auto actionType = sfun::trim(sfun::after(section.name, "Expect"));
        return createComparisonAction(TestActionType::Expectation, std::string{actionType}, section);
    }
    return false;
}

namespace {
bool isValidUnusedSection(const Section& section)
{
    if (section.name == "Section separator")
        return true;
    if (section.name == "Tags")
        return true;

    return false;
}
} //namespace

void Test::readConfig(const fs::path& path, const std::unordered_map<std::string, std::string>& vars)
{
    auto fileStream = std::ifstream{path, std::ios::binary};
    if (!fileStream.is_open())
        throw TestConfigError{fmt::format("Test config file {} doesn't exist", homePathString(path))};

    try {
        auto sections = readSections(fileStream);
        std::transform(
                sections.begin(),
                sections.end(),
                sections.begin(),
                [&](Section& section)
                {
                    section.value = processVariablesSubstitution(section.value, vars);
                    return section;
                });

        if (sections.empty())
            throw TestConfigError{fmt::format("Test config file {} is empty or invalid", homePathString(path))};
        for (const auto& section : sections) {
            if (readParamFromSection(section))
                continue;
            if (readActionFromSection(section))
                continue;
            if (isValidUnusedSection(section))
                continue;
            throw TestConfigError{fmt::format("Unsupported section name: {}", section.name)};
        }
    }
    catch (const std::exception& e) {
        throw TestConfigError{e.what()};
    }

    checkParams();
}

void Test::checkParams()
{
    if (!fs::exists(directory_))
        throw TestConfigError{fmt::format("Specified directory '{}' doesn't exist", homePathString(directory_))};
}

bool Test::createComparisonAction(
        TestActionType actionType,
        const std::string& encodedActionType,
        const Section& section)
{
    if (encodedActionType == "files equal" || encodedActionType == "text files equal" ||
        encodedActionType == "data files equal") {
        createCompareFilesAction(actionType, encodedActionType, section.value);
        return true;
    }
    if (encodedActionType == "exit code") {
        createCompareExitCodeAction(actionType, section.value);
        return true;
    }
    if (encodedActionType == "output") {
        actions_.push_back({CompareOutput{launchActionResult_, section.value}, actionType});
        return true;
    }
    if (encodedActionType == "error output") {
        actions_.push_back({CompareErrorOutput{launchActionResult_, section.value}, actionType});
        return true;
    }
    createCompareFileContentAction(actionType, encodedActionType, section.value);
    return true;
}

void Test::createLaunchAction(const Section& section)
{
    const auto parts = sfun::split(section.name);
    auto uncheckedResult = std::find(parts.begin(), parts.end(), "unchecked") != parts.end();
    auto isShellCommand = std::find(parts.begin(), parts.end(), "process") == parts.end();

    const auto& command = section.value;
    auto shellCommand = [&]() -> std::optional<std::string>
    {
        if (isShellCommand)
            return shellCommand_;
        return std::nullopt;
    };
    actions_.push_back(
            {LaunchProcess{command, directory_, shellCommand(), uncheckedResult, nextAction_},
             TestActionType::RequiredOperation});
}

void Test::createWriteAction(const Section& section)
{
    const auto fileName = sfun::trim(sfun::after(section.name, "Write"));
    const auto path = fs::absolute(directory_) / sfun::makePath(fileName);
    actions_.push_back({WriteFile{path, section.value}, TestActionType::RequiredOperation});
}

void Test::createCompareFilesAction(
        TestActionType actionType,
        const std::string& comparisonType,
        const std::string& filenamesStr)
{
    const auto filenameGroups = readFilenames(filenamesStr, directory_);
    if (filenameGroups.size() != 2)
        throw TestConfigError{"Comparison of files requires exactly two filenames or filename matching regular "
                              "expressions to be specified"};
    auto comparisonMode = sfun::startsWith(comparisonType, "data") ? ComparisonMode::Binary : ComparisonMode::Text;
    actions_.push_back({CompareFiles{filenameGroups[0], filenameGroups[1], comparisonMode}, actionType});
}

void Test::createCompareFileContentAction(
        TestActionType actionType,
        const std::string& filenameStr,
        const std::string& expectedFileContent)
{
    const auto filePath = fs::absolute(directory_) / sfun::makePath(filenameStr);
    actions_.push_back({CompareFileContent{filePath, expectedFileContent}, actionType});
}

void Test::createCompareExitCodeAction(TestActionType actionType, const std::string& expectedExitCodeStr)
{
    auto expectedExitCode = 0;
    try {
        expectedExitCode = std::stoi(expectedExitCodeStr);
    }
    catch (...) {
        throw TestConfigError{"Process exit code must be an integer"};
    }
    actions_.push_back({CompareExitCode{launchActionResult_, expectedExitCode}, actionType});
}

void Test::cleanTestFiles()
{
    if (contents_.empty())
        return;
    auto contentsPaths = std::set<fs::path>{};
    for (const auto& fileNameGroup : contents_) {
        for (const auto& path : fileNameGroup.pathList())
            contentsPaths.insert(path);
    }

    auto paths = getDirectoryContent(directory_);
    std::sort(paths.begin(), paths.end(), std::greater<>{});

    for (const auto& path : paths)
        if (!contentsPaths.count(path))
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
    if (contents_.empty()) {
        auto pathList = getDirectoryContent(directory_);
        std::transform(
                pathList.begin(),
                pathList.end(),
                std::back_inserter(contents_),
                [this](const fs::path& path)
                {
                    return FilenameGroup{sfun::pathString(fs::relative(path, directory_)), directory_};
                });
        return;
    }
    contents_.emplace_back(sfun::pathString(fs::relative(configPath, directory_)), directory_);
}

} //namespace lunchtoast