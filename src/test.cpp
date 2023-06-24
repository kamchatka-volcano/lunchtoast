#include "test.h"
#include "comparefilecontent.h"
#include "comparefiles.h"
#include "constants.h"
#include "errors.h"
#include "launchprocess.h"
#include "sectionsreader.h"
#include "utils.h"
#include "writefile.h"
#include <fmt/format.h>
#include <range/v3/action.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <sfun/functional.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <gsl/util>
#include <algorithm>
#include <fstream>
#include <functional>
#include <sstream>

namespace lunchtoast {
namespace views = ranges::views;
namespace fs = std::filesystem;
using namespace std::string_view_literals;

namespace {
std::vector<FilenameGroup> getDefaultContents(const fs::path& testDirectory)
{
    const auto isDefaultTestFile = [](const fs::path& path)
    {
        return (is_regular_file(path) && sfun::path_string(path.filename()) == hardcoded::testCaseFilename);
    };
    const auto isConfigFile = [](const fs::path& path)
    {
        return (is_regular_file(path) && sfun::path_string(path.filename()) == hardcoded::configFilename);
    };
    const auto toFilenameFroup = [&](const fs::path& path)
    {
        return FilenameGroup{sfun::path_string(fs::relative(path, testDirectory)), testDirectory};
    };

    const auto dirContent = getDirectoryContent(testDirectory);
    return views::concat(
                   dirContent | views::filter(isDefaultTestFile), //
                   dirContent | views::filter(isConfigFile)) |
            views::transform(toFilenameFroup) | ranges::to<std::vector>;
}
} //namespace

Test::Test(
        const fs::path& testCasePath,
        const std::unordered_map<std::string, std::string>& vars,
        std::vector<UserAction> userActions,
        std::string shellCommand,
        bool cleanup)
    : userActions_{std::move(userActions)}
    , shellCommand_(std::move(shellCommand))
    , cleanup_(cleanup)
    , directory_(testCasePath.parent_path())
    , name_(sfun::path_string(directory_.filename()))
    , isEnabled_(true)
    , contents_{getDefaultContents(directory_)}
{
    readTestCase(testCasePath, vars);
    postProcessCleanupConfig(testCasePath);
}

TestResult Test::process()
{
    if (cleanup_)
        cleanTestFiles();

    auto failedActionsMessages = std::vector<std::string>{};
    if (actions_.empty())
        return TestResult::RuntimeError("Test has nothing to check", failedActionsMessages);

    auto testResult = true;

    auto runtimeError = std::optional<std::string>{};
    const auto onActionError = [&](auto&, const std::string& errorInfo)
    {
        testResult = false;
        runtimeError = errorInfo;
    };
    const auto onActionSuccessful = [](auto&) {};

    const auto cleanup = gsl::finally(
            [&]
            {
                if (testResult && cleanup_)
                    cleanTestFiles();
            });

    const auto closeDetachedProcesses = gsl::finally(
            [&]
            {
                for (auto& detachedProcess : detachedProcessList_)
                    while (detachedProcess.running()) {
#ifndef _WIN32
                        auto errorCode = std::error_code{};
                        detachedProcess.terminate(errorCode);
#else
                        const auto id = detachedProcess.id();
                        detachedProcess.detach();
                        runCommand(std::format("taskkill /f /t /pid {}", id));
#endif
                    }
            });

    for (auto& action : actions_) {
        auto actionResult = true;
        const auto onActionFailed = [&](auto&, const std::string& errorInfo)
        {
            actionResult = false;
            testResult = false;
            failedActionsMessages.push_back(errorInfo);
        };

        action.process(onActionSuccessful, onActionFailed, onActionError);

        if (runtimeError)
            return TestResult::RuntimeError(runtimeError.value(), failedActionsMessages);

        auto stopOnFailure =
                (action.type() == TestActionType::Assertion || action.type() == TestActionType::RequiredOperation);

        if (!actionResult && stopOnFailure)
            break;
    }

    return testResult ? TestResult::Success() : TestResult::Failure(failedActionsMessages);
}

std::vector<Section> Test::readParam(const std::vector<Section>& sections)
{
    if (sections.empty())
        return {};

    const auto& section = sections.front();
    if (readParam(name_, "Name", section))
        return sections | views::drop(1) | ranges::to<std::vector>;
    if (readParam(suite_, "Suite", section))
        return sections | views::drop(1) | ranges::to<std::vector>;
    if (readParam(description_, "Description", section))
        return sections | views::drop(1) | ranges::to<std::vector>;
    if (readParam(isEnabled_, "Enabled", section))
        return sections | views::drop(1) | ranges::to<std::vector>;

    auto sectionContents = std::vector<FilenameGroup>{};
    if (readParam(sectionContents, "Contents", section)) {
        contents_ = views::concat(contents_, sectionContents) | ranges::to<std::vector>;
        return sections | views::drop(1) | ranges::to<std::vector>;
    }
    return sections;
}

namespace {
template<typename TAction>
int countActions(const std::vector<TestAction>& actions)
{
    return gsl::narrow_cast<int>(std::ranges::count_if(
            actions,
            [](const auto& action)
            {
                return action.template is<TAction>();
            }));
}
} //namespace

std::vector<Section> Test::readAction(
        const std::vector<Section>& sections,
        const std::unordered_map<std::string, std::string>& vars)
{
    if (sections.empty())
        return sections;

    const auto& section = sections.front();
    for (const auto& userAction : userActions_.get()) {
        auto command = userAction.makeCommand(section.name, vars, section.value);
        if (command.has_value()) {
            actions_.push_back(
                    {LaunchProcess{
                             command.value(),
                             directory_,
                             shellCommand_,
                             userAction.makeProcessResultCheckModeSet(vars, section.value),
                             countActions<LaunchProcess>(actions_)},
                     userAction.actionType()});
            return sections | views::drop(1) | ranges::to<std::vector>;
        }
    }

    if (section.name.starts_with("Launch")) {
        return createLaunchAction(section, sections | views::drop(1) | ranges::to<std::vector>);
    }
    if (section.name.starts_with("Write")) {
        createWriteAction(section);
        return sections | views::drop(1) | ranges::to<std::vector>;
    }
    if (section.name.starts_with("Wait")) {
        actions_.emplace_back(makeWaitAction(section), TestActionType::RequiredOperation);
        return sections | views::drop(1) | ranges::to<std::vector>;
    }
    if (section.name.starts_with("Assert")) {
        auto actionType = sfun::trim(sfun::after(section.name, "Assert"));
        createComparisonAction(TestActionType::Assertion, std::string{actionType}, section);
        return sections | views::drop(1) | ranges::to<std::vector>;
    }
    if (section.name.starts_with("Expect")) {
        auto actionType = sfun::trim(sfun::after(section.name, "Expect"));
        createComparisonAction(TestActionType::Expectation, std::string{actionType}, section);
        return sections | views::drop(1) | ranges::to<std::vector>;
    }
    return sections;
}

namespace {
std::vector<Section> readValidUnusedSection(const std::vector<Section>& sections)
{
    if (sections.empty())
        return {};

    const auto& section = sections.front();
    if (section.name == "Section separator")
        return sections | views::drop(1) | ranges::to<std::vector>;
    if (section.name == "Tags")
        return sections | views::drop(1) | ranges::to<std::vector>;

    return sections;
}
} //namespace

void Test::readTestCase(const fs::path& path, const std::unordered_map<std::string, std::string>& vars)
{
    auto fileStream = std::ifstream{path, std::ios::binary};
    if (!fileStream.is_open())
        throw TestConfigError{fmt::format("Test case file {} doesn't exist", homePathString(path))};

    try {
        auto sections = readSections(fileStream);
        const auto setSectionVars = [&](Section& section)
        {
            section.value = processVariablesSubstitution(section.value, vars);
            return section;
        };
        sections = sections | views::transform(setSectionVars) | ranges::to<std::vector>;

        if (sections.empty())
            throw TestConfigError{fmt::format("Test case file {} is empty or invalid", homePathString(path))};

        auto sectionsCount = sfun::ssize_t{0};
        while (std::ssize(sections) != sectionsCount) {
            sectionsCount = std::ssize(sections);
            sections = readParam(sections);
            sections = readAction(sections, vars);
            sections = readValidUnusedSection(sections);
        }
        if (!sections.empty())
            throw TestConfigError{fmt::format("Unsupported section name: {}", sections.front().name)};
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

void Test::createComparisonAction(
        TestActionType actionType,
        const std::string& encodedActionType,
        const Section& section)
{
    if (encodedActionType == "files equal" || encodedActionType == "text files equal" ||
        encodedActionType == "data files equal")
        createCompareFilesAction(actionType, encodedActionType, section.value);
    else
        createCompareFileContentAction(actionType, encodedActionType, section.value);
}
namespace {
std::tuple<std::set<ProcessResultCheckMode>, TestActionType> getResultCheckMode(const std::vector<Section>& sections)
{
    auto makeExitCodeCheck = [](const std::string& str)
    {
        if (sfun::trim(str) == "*" || sfun::trim(str) == "any")
            return ProcessResultCheckMode::ExitCode{};
        try {
            return ProcessResultCheckMode::ExitCode{std::stoi(str)};
        }
        catch (...) {
            throw TestConfigError{fmt::format("Invalid exit code '{}', value must be integer", str)};
        }
    };

    auto actionType = std::optional<TestActionType>();
    auto updateActionType = [&](TestActionType newActionType)
    {
        if (!actionType.has_value())
            actionType = newActionType;
        else {
            if (actionType.value() != newActionType)
                throw TestConfigError{"Launched process action result checks must use either Assert or Expect"};
        }
    };
    auto checkModes = std::vector<ProcessResultCheckMode>{};
    for (const auto& section : sections) {
        if (section.name == "Assert exit code") {
            checkModes.emplace_back(makeExitCodeCheck(section.value));
            updateActionType(TestActionType::Assertion);
        }
        else if (section.name == "Expect exit code") {
            checkModes.emplace_back(makeExitCodeCheck(section.value));
            updateActionType(TestActionType::Expectation);
        }
        else if (section.name == "Assert output") {
            checkModes.emplace_back(ProcessResultCheckMode::Output{section.value});
            updateActionType(TestActionType::Assertion);
        }
        else if (section.name == "Expect output") {
            checkModes.emplace_back(ProcessResultCheckMode::Output{section.value});
            updateActionType(TestActionType::Expectation);
        }
        else if (section.name == "Assert error output") {
            checkModes.emplace_back(ProcessResultCheckMode::ErrorOutput{section.value});
            updateActionType(TestActionType::Assertion);
        }
        else if (section.name == "Expect error output") {
            checkModes.emplace_back(ProcessResultCheckMode::ErrorOutput{section.value});
            updateActionType(TestActionType::Expectation);
        }
        else
            break;
    }
    if (std::ssize(checkModes) > 3)
        throw TestConfigError{
                "Launched process action supports maximum 3 result checks (exit code, output and error output)"};

    auto result = std::set<ProcessResultCheckMode>{checkModes.begin(), checkModes.end()};
    if (std::ssize(checkModes) != std::ssize(result))
        throw TestConfigError{
                "Launched process action result checks type must be unique (exit code, output and error output)"};

    return {result, actionType.has_value() ? actionType.value() : TestActionType::Assertion};
}

} //namespace

std::vector<Section> Test::createLaunchAction(const Section& section, const std::vector<Section>& nextSections)
{
    const auto parts = sfun::split(section.name);
    const auto shellCommand = [&]() -> std::optional<std::string>
    {
        if (std::ranges::find(parts, "process") == parts.end())
            return shellCommand_;
        return std::nullopt;
    };
    const auto isDetached = (std::ranges::find(parts, "detached") != parts.end());
    const auto skipReadingOutput = //
            contains(parts, {"ignore"sv, "output"sv}) || contains(parts, {"ignoring"sv, "output"sv});

    const auto [checkModeSet, actionType, foundCheckModesCount] = [&]
    {
        const auto [checkModeSetRes, actionTypeRes] = getResultCheckMode(nextSections);
        if (checkModeSetRes.empty())
            return std::make_tuple(
                    views::single(ProcessResultCheckMode::ExitCode{0}) | ranges::to<std::set<ProcessResultCheckMode>>,
                    TestActionType::Assertion,
                    std::ssize(checkModeSetRes));
        return std::make_tuple(checkModeSetRes, actionTypeRes, std::ssize(checkModeSetRes));
    }();

    actions_.push_back(
            {LaunchProcess{
                     std::string{sfun::trim(section.value)},
                     directory_,
                     shellCommand(),
                     checkModeSet,
                     countActions<LaunchProcess>(actions_),
                     isDetached ? &detachedProcessList_ : nullptr,
                     skipReadingOutput},
             actionType});

    return nextSections | views::drop(foundCheckModesCount) | ranges::to<std::vector>;
}

void Test::createWriteAction(const Section& section)
{
    const auto fileName = sfun::trim(sfun::after(section.name, "Write"));
    const auto path = fs::absolute(directory_) / sfun::make_path(fileName);
    actions_.push_back({WriteFile{path, section.value}, TestActionType::RequiredOperation});
}

void Test::createCompareFilesAction(
        TestActionType actionType,
        const std::string& comparisonType,
        const std::string& filenamesStr)
{
    const auto files = readFilenames(filenamesStr, directory_);
    if (std::ssize(files) != 2)
        throw TestConfigError{"Comparison of files requires exactly two file names"};
    const auto comparisonMode = comparisonType.starts_with("data") ? ComparisonMode::Binary : ComparisonMode::Text;
    actions_.push_back({CompareFiles{files[0], files[1], comparisonMode}, actionType});
}

void Test::createCompareFileContentAction(
        TestActionType actionType,
        const std::string& filenameStr,
        const std::string& expectedFileContent)
{
    const auto filePath = fs::absolute(directory_) / sfun::make_path(filenameStr);
    actions_.push_back(
            {CompareFileContent{filePath, expectedFileContent, directory_, countActions<CompareFileContent>(actions_)},
             actionType});
}

void Test::cleanTestFiles()
{
    if (contents_.empty())
        return;
    const auto toPathList = [](const FilenameGroup& group)
    {
        return group.pathList();
    };
    const auto contentsPathsList = contents_ | views::transform(toPathList) | ranges::to<std::vector>;
    const auto contentsPaths = contentsPathsList | views::join | ranges::to<std::set>;

    const auto notInContentPaths = [&](const fs::path& path)
    {
        return !contentsPaths.count(path);
    };
    const auto paths = getDirectoryContent(directory_) | ranges::actions::sort(std::greater<>{});
    const auto pathsNotInContent = paths | views::filter(notInContentPaths) | ranges::to<std::vector>;
    std::ranges::for_each(
            pathsNotInContent,
            [](const auto& path)
            {
                fs::remove(path);
            });
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
    param = readFilenameGroups(section.value, directory_);
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

void Test::postProcessCleanupConfig(const fs::path& testCasePath)
{
    if (contents_.empty()) {
        const auto pathToFilenameGroup = [this](const fs::path& path)
        {
            return FilenameGroup{sfun::path_string(fs::relative(path, directory_)), directory_};
        };
        const auto pathList = getDirectoryContent(directory_);
        contents_ = pathList | views::transform(pathToFilenameGroup) | ranges::to<std::vector>;
        return;
    }
    contents_.emplace_back(sfun::path_string(fs::relative(testCasePath, directory_)), directory_);
}

} //namespace lunchtoast