#include "launchprocess.h"
#include "errors.h"
#include "testaction.h"
#include "utils.h"
#include <fmt/format.h>
#include <sfun/functional.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <sfun/utility.h>
#include <sfun/wstringconv.h>
#include <boost/process.hpp>
#include <filesystem>
#include <utility>

namespace lunchtoast {

namespace proc = boost::process;
namespace fs = std::filesystem;

LaunchProcess::LaunchProcess(
        std::string command,
        fs::path workingDir,
        std::optional<std::string> shellCommand,
        std::set<ProcessResultCheckMode> checkModeSet,
        std::optional<TestAction>& nextAction)
    : command_{std::move(command)}
    , workingDir_{std::move(workingDir)}
    , shellCommand_{std::move(shellCommand)}
    , checkModeSet_{std::move(checkModeSet)}
    , nextAction_{&nextAction}
{
    auto paths = boost::this_process::path();
}

namespace {
auto osArgs(const std::vector<std::string_view>& args)
{
#ifndef _WIN32
    return args;
#else
    auto result = std::vector<std::wstring>{};
    std::transform(
            args.begin(),
            args.end(),
            std::back_inserter(result),
            [](const std::string_view& arg)
            {
                return sfun::toWString(arg);
            });
    return result;
#endif
}

std::optional<std::tuple<std::string_view, std::vector<std::string_view>>> parseCommand(
        const std::optional<std::string>& shellCommand,
        const std::string& command)
{
    auto cmdParts = shellCommand.has_value() ? splitCommand(shellCommand.value()) : splitCommand(command);
    if (cmdParts.empty())
        return std::nullopt;

    auto processCmd = cmdParts[0];
    cmdParts.erase(cmdParts.begin());
    auto processCmdParts = sfun::split(processCmd);
    if (sfun::ssize(processCmdParts) > 1) {
        processCmd = processCmdParts[0];
        std::copy(processCmdParts.begin() + 1, processCmdParts.end(), std::inserter(cmdParts, cmdParts.begin()));
    }
    if (shellCommand.has_value())
        cmdParts.push_back(command);

    return std::tuple{processCmd, cmdParts};
}

auto makeCheckModeVisitor(const LaunchProcessResult& result, const std::string& command)
{
    return sfun::overloaded{
            [&](const ProcessResultCheckMode::ExitCode& exitCode)
            {
                if (result.exitCode != exitCode.value)
                    return TestActionResult::Failure(fmt::format(
                            "Launched process '{}' returned unexpected exit code {}, expected exit code: {}",
                            command,
                            result.exitCode,
                            exitCode.value));
                return TestActionResult::Success();
            },
            [&](const ProcessResultCheckMode::Output& output)
            {
                if (result.output != output.value)
                    return TestActionResult::Failure(fmt::format(
                            "Launched process '{}' returned unexpected output '{}', expected output: '{}'",
                            command,
                            result.output,
                            output.value));
                return TestActionResult::Success();
            },
            [&](const ProcessResultCheckMode::ErrorOutput& output)
            {
                if (result.errorOutput != output.value)
                    return TestActionResult::Failure(fmt::format(
                            "Launched process '{}' returned unexpected error output '{}', expected error output: '{}'",
                            command,
                            result.errorOutput,
                            output.value));
                return TestActionResult::Success();
            }};
}

LaunchProcessResult startProcess(
        const boost::filesystem::path& cmd,
        const std::vector<std::string_view>& cmdArgs,
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
            result.output += outLine;

        auto errLine = std::string{};
        while (std::getline(stderrStream, errLine) && !errLine.empty())
            result.errorOutput += errLine;
    }
    process.wait();

    result.exitCode = process.exit_code();
    return result;
}

} //namespace

TestActionResult LaunchProcess::operator()()
{
    auto res = parseCommand(shellCommand_, command_);
    if (!res)
        return TestActionResult::Failure("Can't launch an empty shell command");

    auto& [cmdName, cmdArgs] = res.value();

    auto paths = boost::this_process::path();
    paths.emplace_back(workingDir_);
    auto cmd = proc::search_path(std::string{cmdName}, paths);
    if (cmd.empty())
        throw TestConfigError{fmt::format("Couldn't find the executable of a command '{}'", cmdName)};

    result_ = startProcess(cmd, cmdArgs, workingDir_);

    if (nextAction_->has_value() && nextAction_->value().is<CompareExitCode>())
        checkModeSet_.clear();

    if (checkModeSet_.empty())
        return TestActionResult::Success();

    for (const auto& checkMode : checkModeSet_) {
        auto result = std::visit(makeCheckModeVisitor(result_, command_), checkMode.value);
        if (!result.isSuccessful())
            return result;
    }
    return TestActionResult::Success();
}

LaunchProcessResult LaunchProcess::result() const
{
    return result_;
}

} //namespace lunchtoast