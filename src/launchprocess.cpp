#include "launchprocess.h"
#include "errors.h"
#include "testaction.h"
#include "utils.h"
#include <fmt/format.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <sfun/utility.h>
#include <sfun/utfconv.h>
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
        bool uncheckedResult,
        std::optional<TestAction>& nextAction)
    : command_(std::move(command))
    , workingDir_(std::move(workingDir))
    , shellCommand_(std::move(shellCommand))
    , uncheckedResult_(uncheckedResult)
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
                return sfun::toUtf16(arg);
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

    auto stdoutStream = proc::ipstream{};
    auto stderrStream = proc::ipstream{};
    auto process = proc::child{
            cmd,
            proc::args(osArgs(cmdArgs)),
            proc::start_dir = sfun::pathString(workingDir_),
            proc::std_out > stdoutStream,
            proc::std_err > stderrStream};

    while (process.running()) {
        auto outLine = std::string{};
        while (std::getline(stdoutStream, outLine) && !outLine.empty())
            result_.output += outLine;

        auto errLine = std::string{};
        while (std::getline(stderrStream, errLine) && !errLine.empty())
            result_.errorOutput += errLine;
    }
    process.wait();

    result_.exitCode = process.exit_code();

    if (nextAction_->has_value() && nextAction_->value().is<CompareExitCode>())
        uncheckedResult_ = true;

    if (uncheckedResult_)
        return TestActionResult::Success();

    if (result_.exitCode != 0)
        return TestActionResult::Failure(fmt::format("Launched process '{}' returned non-zero exit code", command_));
    else
        return TestActionResult::Success();
}

LaunchProcessResult LaunchProcess::result() const
{
    return result_;
}

} //namespace lunchtoast