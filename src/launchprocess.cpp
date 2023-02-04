#include "launchprocess.h"
#include "errors.h"
#include "testaction.h"
#include "utils.h"
#include <fmt/format.h>
#include <sfun/string_utils.h>
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

auto pathToString(const std::filesystem::path& path)
{
#ifndef _WIN32
    return path.string();
#else
    return path.wstring();
#endif
}

} //namespace

TestActionResult LaunchProcess::operator()()
{
    auto cmdParts = shellCommand_.has_value() ? splitCommand(shellCommand_.value()) : splitCommand(command_);
    auto paths = boost::this_process::path();
    paths.emplace_back(workingDir_);
    auto cmd = proc::search_path(std::string{cmdParts[0]}, paths);
    if (cmd.empty())
        throw TestConfigError{fmt::format("Couldn't find the executable of a command '{}'", cmdParts[0])};

    cmdParts.erase(cmdParts.begin());
    if (shellCommand_.has_value())
        cmdParts.push_back(command_);

    auto stdoutStream = proc::ipstream{};
    auto stderrStream = proc::ipstream{};
    auto process = proc::child{
            cmd,
            proc::args(cmdParts),
            proc::start_dir = pathToString(workingDir_),
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