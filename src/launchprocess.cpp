#include "launchprocess.h"
#include "constants.h"
#include "errors.h"
#include "testaction.h"
#include "utils.h"
#include <fmt/format.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <sfun/functional.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <sfun/utility.h>
#include <sfun/wstringconv.h>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <filesystem>
#include <fstream>
#include <future>
#include <utility>

namespace lunchtoast {
namespace views = ranges::views;
namespace proc = boost::process;
namespace fs = std::filesystem;

LaunchProcess::LaunchProcess(
        std::string command,
        fs::path workingDir,
        std::optional<std::string> shellCommand,
        std::set<ProcessResultCheckMode> checkModeSet,
        int actionIndex,
        std::vector<boost::process::child>* detachedProcessList)
    : command_{std::move(command)}
    , workingDir_{std::move(workingDir)}
    , shellCommand_{std::move(shellCommand)}
    , checkModeSet_{std::move(checkModeSet)}
    , actionIndex_{actionIndex}
    , detachedProcessList_{detachedProcessList}
{
    auto paths = boost::this_process::path();
}

namespace {
auto osArgs(const std::vector<std::string>& args)
{
#ifndef _WIN32
    return args;
#else
    const auto toWString = [](const std::string& arg)
    {
        return sfun::to_wstring(arg);
    };
    return args | views::transform(toWString) | ranges::to<std::vector>;
#endif
}

std::tuple<std::string, std::vector<std::string>> parseShellCommand(
        const std::string& shellCommand,
        const std::string& command)
{
    if (shellCommand.find('\n') != std::string::npos)
        throw TestConfigError{fmt::format("Can't launch a command with a newline character: {}", shellCommand)};

    const auto shellCmdParts = splitCommand(shellCommand);
    if (shellCmdParts.empty())
        throw TestConfigError{"Can't launch the process with an empty command"};
    if (command.find('\n') != std::string::npos)
        throw TestConfigError{fmt::format("Can't launch a command with a newline character: {}", command)};

    const auto& shellExec = shellCmdParts.front();
    const auto shellArgs = shellCmdParts | views::drop(1);
    const auto args = views::concat(shellArgs, views::single(command)) | ranges::to<std::vector>;
    return std::tuple{shellExec, args};
}

std::tuple<std::string, std::vector<std::string>> parseCommand(const std::string& command)
{
    if (command.find('\n') != std::string::npos)
        throw TestConfigError{fmt::format("Can't launch a command with a newline character: {}", command)};

    auto cmdParts = splitCommand(command);
    if (cmdParts.empty())
        throw TestConfigError{"Can't launch the process with an empty command"};

    const auto& processExec = cmdParts.front();
    const auto args = cmdParts | views::drop(1) | ranges::to<std::vector>;
    return std::tuple{processExec, args};
}

std::string failureReportFilename(int actionIndex)
{
    return fmt::format(hardcoded::launchFailureReportFilename, actionIndex);
}

auto makeCheckModeVisitor(const LaunchProcessResult& result, const std::string& command, int actionIndex)
{
    return sfun::overloaded{
            [&, actionIndex = actionIndex](const ProcessResultCheckMode::ExitCode& exitCode)
            {
                if (!exitCode.value.has_value())
                    return TestActionResult::Success();

                if (result.exitCode != exitCode.value)
                    return TestActionResult::Failure(fmt::format(
                            "Launched process '{}' returned unexpected exit code {}. More info in {}",
                            command,
                            result.exitCode,
                            failureReportFilename(actionIndex)));
                return TestActionResult::Success();
            },
            [&, actionIndex = actionIndex](const ProcessResultCheckMode::Output& output)
            {
                if (result.output != normalizeLineEndings(output.value))
                    return TestActionResult::Failure(fmt::format(
                            "Launched process '{}' returned unexpected output. More info in {}",
                            command,
                            failureReportFilename(actionIndex)));
                return TestActionResult::Success();
            },
            [&, actionIndex = actionIndex](const ProcessResultCheckMode::ErrorOutput& output)
            {
                if (result.errorOutput != normalizeLineEndings(output.value))
                    return TestActionResult::Failure(fmt::format(
                            "Launched process '{}' returned unexpected error output. More info in {}",
                            command,
                            failureReportFilename(actionIndex)));
                return TestActionResult::Success();
            }};
}

LaunchProcessResult startProcess(
        const boost::filesystem::path& cmd,
        const std::vector<std::string>& cmdArgs,
        const std::filesystem::path& workingDir)
{
    auto ios = boost::asio::io_service{};
    auto stdoutData = std::future<std::string>{};
    auto stderrData = std::future<std::string>{};
    auto process = proc::child{
            cmd,
            proc::args(osArgs(cmdArgs)),
            proc::start_dir = sfun::path_string(workingDir),
            proc::std_out > stdoutData,
            proc::std_err > stderrData,
            ios};

    ios.run();
    process.wait();
    if (process.running())
        process.terminate();

    return {
        .exitCode = process.exit_code(),
        .output = normalizeLineEndings(stdoutData.get()),
        .errorOutput = normalizeLineEndings(stderrData.get())
    };
}

proc::child startDetachedProcess(
        const boost::filesystem::path& cmd,
        const std::vector<std::string>& cmdArgs,
        const std::filesystem::path& workingDir)
{
    return proc::child{
            cmd,
            proc::args(osArgs(cmdArgs)),
            proc::start_dir = sfun::path_string(workingDir),
            proc::std_out > proc::null,
            proc::std_err > proc::null};
}

struct ExpectedLaunchProcessResult {
    std::optional<int> exitCode;
    std::optional<std::string> output;
    std::optional<std::string> errorOutput;
};

auto makeCheckModeVisitorSettingExpectedResult(ExpectedLaunchProcessResult& expectedResult)
{
    return sfun::overloaded{
            [&](const ProcessResultCheckMode::ExitCode& exitCode)
            {
                expectedResult.exitCode = exitCode.value;
            },
            [&](const ProcessResultCheckMode::Output& output)
            {
                expectedResult.output = normalizeLineEndings(output.value);
            },
            [&](const ProcessResultCheckMode::ErrorOutput& output)
            {
                expectedResult.errorOutput = normalizeLineEndings(output.value);
            }};
}

std::string generateLaunchFailureReport(
        std::string_view command,
        const LaunchProcessResult& result,
        const std::set<ProcessResultCheckMode>& checkModeSet)
{
    auto expectedResult = ExpectedLaunchProcessResult{};
    const auto updateExpectedResult = makeCheckModeVisitorSettingExpectedResult(expectedResult);
    for (const auto& checkMode : checkModeSet)
        std::visit(updateExpectedResult, checkMode.value);

    auto report = fmt::format("-Command: {}\n", command);

    report += fmt::format("-Exit code: {}\n", result.exitCode);
    if (expectedResult.exitCode.has_value())
        report += fmt::format("-Expected exit code: {}\n", expectedResult.exitCode.value());

    report += fmt::format("-Output:\n{}---\n", result.output.empty() ? "" : result.output + "\n");
    if (expectedResult.output.has_value())
        report += fmt::format(
                "-Expected output:\n{}---\n",
                expectedResult.output.value().empty() ? "" : expectedResult.output.value() + "\n");

    report += fmt::format("-Error output:\n{}---\n", result.errorOutput.empty() ? "" : result.errorOutput + "\n");
    if (expectedResult.errorOutput.has_value())
        report += fmt::format(
                "-Expected error output:\n{}---\n",
                expectedResult.errorOutput.value().empty() ? "" : expectedResult.errorOutput.value() + "\n");

    return report;
}

} //namespace

TestActionResult LaunchProcess::operator()() const
{
    const auto [cmdName, cmdArgs] = shellCommand_.has_value() ? parseShellCommand(shellCommand_.value(), command_)
                                                              : parseCommand(command_);
    const auto currentPath = boost::this_process::path();
    const auto path = views::concat(currentPath, views::single(workingDir_)) | ranges::to<std::vector>;
    const auto cmd = proc::search_path(std::string{cmdName}, path);
    if (cmd.empty())
        throw TestConfigError{fmt::format("Couldn't find the executable of a command '{}'", cmdName)};

    if (detachedProcessList_) {
        auto process = startDetachedProcess(cmd, cmdArgs, workingDir_);
        if (!process.valid())
            throw TestConfigError{fmt::format("Couldn't start the process '{}'", command_)};
        detachedProcessList_->emplace_back(std::move(process));
        return TestActionResult::Success();
    }

    const auto launchResult = startProcess(cmd, cmdArgs, workingDir_);
    if (checkModeSet_.empty())
        return TestActionResult::Success();

    for (const auto& checkMode : checkModeSet_) {
        const auto result = std::visit(makeCheckModeVisitor(launchResult, command_, actionIndex_), checkMode.value);
        if (!result.isSuccessful()) {
            auto failureReport = generateLaunchFailureReport(
                    cmd.string() + " " + sfun::join(cmdArgs, " "),
                    launchResult,
                    checkModeSet_);
            auto failureReportFile = std::ofstream{workingDir_ / failureReportFilename(actionIndex_)};
            failureReportFile << failureReport;
            return result;
        }
    }
    return TestActionResult::Success();
}

} //namespace lunchtoast