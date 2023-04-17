#include "launchprocess.h"
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
#include <boost/process.hpp>
#include <filesystem>
#include <fstream>
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
        int actionIndex)
    : command_{std::move(command)}
    , workingDir_{std::move(workingDir)}
    , shellCommand_{std::move(shellCommand)}
    , checkModeSet_{std::move(checkModeSet)}
    , actionIndex_{actionIndex}
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
        return sfun::toWString(arg);
    };
    return args | views::transform(toWString) | ranges::to<std::vector>;
#endif
}

std::tuple<std::string, std::vector<std::string>> parseShellCommand(
        const std::string& shellCommand,
        const std::string& command)
{
    const auto shellCmdParts = splitCommand(shellCommand);
    if (shellCmdParts.empty())
        throw TestConfigError{"Can't launch the process with an empty command"};

    const auto& shellExec = shellCmdParts.front();
    const auto shellArgs = shellCmdParts | views::drop(1);
    const auto args = views::concat(shellArgs, views::single(command)) | ranges::to<std::vector>;
    return std::tuple{shellExec, args};
}

std::tuple<std::string, std::vector<std::string>> parseCommand(const std::string& command)
{
    auto cmdParts = splitCommand(command);
    if (cmdParts.empty())
        throw TestConfigError{"Can't launch the process with an empty command"};

    const auto& processExec = cmdParts.front();
    const auto args = cmdParts | views::drop(1) | ranges::to<std::vector>;
    return std::tuple{processExec, args};
}

std::string failureReportFilename(int actionIndex)
{
    return fmt::format("launch_failure_{}.txt", actionIndex);
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
                if (result.output != output.value)
                    return TestActionResult::Failure(fmt::format(
                            "Launched process '{}' returned unexpected output. More info in {}",
                            command,
                            failureReportFilename(actionIndex)));
                return TestActionResult::Success();
            },
            [&, actionIndex = actionIndex](const ProcessResultCheckMode::ErrorOutput& output)
            {
                if (result.errorOutput != output.value)
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
    auto stdoutStream = proc::ipstream{};
    auto stderrStream = proc::ipstream{};
    auto process = proc::child{
            cmd,
            proc::args(osArgs(cmdArgs)),
            proc::start_dir = sfun::pathString(workingDir),
            proc::std_out > stdoutStream,
            proc::std_err > stderrStream};

    auto result = LaunchProcessResult{};
    while (process.running()) {
        auto outLine = std::string{};
        while (std::getline(stdoutStream, outLine) && !outLine.empty())
            result.output += outLine + "\n";

        auto errLine = std::string{};
        while (std::getline(stderrStream, errLine) && !errLine.empty())
            result.errorOutput += errLine + "\n";
    }
    process.wait();

    if (sfun::endsWith(result.output, "\n"))
        result.output.pop_back();
    if (sfun::endsWith(result.errorOutput, "\n"))
        result.errorOutput.pop_back();

    result.exitCode = process.exit_code();
    return result;
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
                expectedResult.output = output.value;
            },
            [&](const ProcessResultCheckMode::ErrorOutput& output)
            {
                expectedResult.errorOutput = output.value;
            }};
}

std::string generateLaunchFailureReport(
        std::string_view command,
        const LaunchProcessResult& result,
        std::set<ProcessResultCheckMode>& checkModeSet)
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

TestActionResult LaunchProcess::operator()()
{
    const auto [cmdName, cmdArgs] = shellCommand_.has_value() ? parseShellCommand(shellCommand_.value(), command_)
                                                              : parseCommand(command_);
    const auto currentPath = boost::this_process::path();
    const auto path = views::concat(currentPath, views::single(workingDir_)) | ranges::to<std::vector>;
    const auto cmd = proc::search_path(std::string{cmdName}, path);
    if (cmd.empty())
        throw TestConfigError{fmt::format("Couldn't find the executable of a command '{}'", cmdName)};

    result_ = startProcess(cmd, cmdArgs, workingDir_);

    if (checkModeSet_.empty())
        return TestActionResult::Success();

    for (const auto& checkMode : checkModeSet_) {
        const auto result = std::visit(makeCheckModeVisitor(result_, command_, actionIndex_), checkMode.value);
        if (!result.isSuccessful()) {
            auto failureReport =
                    generateLaunchFailureReport(cmd.string() + " " + sfun::join(cmdArgs, " "), result_, checkModeSet_);
            auto failureReportFile = std::ofstream{workingDir_ / failureReportFilename(actionIndex_)};
            failureReportFile << failureReport;
            return result;
        }
    }
    return TestActionResult::Success();
}

LaunchProcessResult LaunchProcess::result() const
{
    return result_;
}

} //namespace lunchtoast