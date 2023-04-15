#include "test.h"
#include "comparefilecontent.h"
#include "comparefiles.h"
#include "errors.h"
#include "launchprocess.h"
#include "sectionsreader.h"
#include "utils.h"
#include "writefile.h"
#include <fmt/format.h>
#include <sfun/functional.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <gsl/util>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace lunchtoast {
namespace fs = std::filesystem;

Test::Test(
        const fs::path& testCasePath,
        const std::unordered_map<std::string, std::string>& vars,
        std::vector<UserAction> userActions,
        std::string shellCommand,
        bool cleanup)
    : userActions_{std::move(userActions)}
    , name_(sfun::pathString(testCasePath.stem()))
    , directory_(testCasePath.parent_path())
    , shellCommand_(std::move(shellCommand))
    , isEnabled_(true)
    , cleanup_(cleanup)
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

std::vector<Section> Test::readParam(const std::vector<Section>& sections)
{
    if (sections.empty())
        return {};

    const auto& section = sections.front();
    if (readParam(name_, "Name", section))
        return {std::next(sections.begin()), sections.end()};
    if (readParam(suite_, "Suite", section))
        return {std::next(sections.begin()), sections.end()};
    if (readParam(description_, "Description", section))
        return {std::next(sections.begin()), sections.end()};
    if (readParam(directory_, "Directory", section))
        return {std::next(sections.begin()), sections.end()};
    if (readParam(isEnabled_, "Enabled", section))
        return {std::next(sections.begin()), sections.end()};
    if (readParam(contents_, "Contents", section))
        return {std::next(sections.begin()), sections.end()};

    return sections;
}

namespace {
int countLaunchProcessActions(const std::vector<TestAction>& actions)
{
    return gsl::narrow_cast<int>(std::count_if(
            actions.begin(),
            actions.end(),
            [](const auto& action)
            {
                return action.template is<LaunchProcess>();
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
    for (const auto& userAction : userActions_) {
        auto command = userAction.makeCommand(section.name, vars, section.value);
        if (command.has_value()) {
            actions_.push_back(
                    {LaunchProcess{
                             command.value(),
                             directory_,
                             shellCommand_,
                             userAction.makeProcessResultCheckModeSet(vars, section.value),
                             countLaunchProcessActions(actions_)},
                     userAction.actionType()});
            return {std::next(sections.begin()), sections.end()};
        }
    }

    if (sfun::startsWith(section.name, "Launch")) {
        return createLaunchAction(section, {std::next(sections.begin()), sections.end()});
    }
    if (sfun::startsWith(section.name, "Write")) {
        createWriteAction(section);
        return {std::next(sections.begin()), sections.end()};
    }
    if (sfun::startsWith(section.name, "Assert")) {
        auto actionType = sfun::trim(sfun::after(section.name, "Assert"));
        createComparisonAction(TestActionType::Assertion, std::string{actionType}, section);
        return {std::next(sections.begin()), sections.end()};
    }
    if (sfun::startsWith(section.name, "Expect")) {
        auto actionType = sfun::trim(sfun::after(section.name, "Expect"));
        createComparisonAction(TestActionType::Expectation, std::string{actionType}, section);
        return {std::next(sections.begin()), sections.end()};
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
        return {std::next(sections.begin()), sections.end()};
    if (section.name == "Tags")
        return {std::next(sections.begin()), sections.end()};

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
            throw TestConfigError{fmt::format("Test case file {} is empty or invalid", homePathString(path))};

        auto sectionsCount = sfun::ssize_t{0};
        while (sfun::ssize(sections) != sectionsCount) {
            sectionsCount = sfun::ssize(sections);
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

    auto checkModes = std::vector<ProcessResultCheckMode>{};
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
    if (checkModes.size() > 3)
        throw TestConfigError{
                "Launched process action supports maximum 3 result checks (exit code, output and error output)"};

    auto result = std::set<ProcessResultCheckMode>{checkModes.begin(), checkModes.end()};
    if (checkModes.size() != result.size())
        throw TestConfigError{
                "Launched process action result checks type must be unique (exit code, output and error output)"};

    return {result, actionType.has_value() ? actionType.value() : TestActionType::Assertion};
}

} //namespace

std::vector<Section> Test::createLaunchAction(const Section& section, const std::vector<Section>& nextSections)
{
    const auto parts = sfun::split(section.name);

    auto shellCommand = [&]() -> std::optional<std::string>
    {
        if (std::find(parts.begin(), parts.end(), "process") == parts.end())
            return shellCommand_;
        return std::nullopt;
    };

    auto [checkModeSet, actionType] = getResultCheckMode(nextSections);
    auto foundCheckModesCount = sfun::ssize(checkModeSet);

    if (checkModeSet.empty()) {
        checkModeSet.emplace(ProcessResultCheckMode::ExitCode{0});
        actionType = TestActionType::Assertion;
    }
    actions_.push_back(
            {LaunchProcess{
                     section.value,
                     directory_,
                     shellCommand(),
                     checkModeSet,
                     countLaunchProcessActions(actions_)},
             actionType});

    if (nextSections.empty())
        return {};
    return {std::next(nextSections.begin(), foundCheckModesCount), nextSections.end()};
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

void Test::postProcessCleanupConfig(const fs::path& testCasePath)
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
    contents_.emplace_back(sfun::pathString(fs::relative(testCasePath, directory_)), directory_);
}

} //namespace lunchtoast