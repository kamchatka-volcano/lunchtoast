#include "launchprocess.h"
#include "utils.h"
#include <sfun/string_utils.h>
#include <fmt/format.h>
#include <range/v3/view.hpp>
#include <boost/process.hpp>
#include <utility>

namespace lunchtoast{

namespace proc = boost::process;
namespace fs = std::filesystem;

LaunchProcess::LaunchProcess(
        std::string command,
        fs::path workingDir,
        std::string shellCommand,
        bool uncheckedResult,
        bool silently)
    : command_(std::move(command))
    , workingDir_(std::move(workingDir))
    , shellCommand_(std::move(shellCommand))
    , uncheckedResult_(uncheckedResult)
    , silently_(silently)
{
}

TestActionType LaunchProcess::type() const
{
    return TestActionType::RequiredOperation;
}

TestActionResult LaunchProcess::process()
{
    auto env = boost::this_process::environment();
    env["PATH"] += workingDir_.string();

    auto result = 0;
    if (!shellCommand_.empty()) {
        auto shellCmdParts = splitCommand(shellCommand_);
        auto shellCmd = shellCmdParts[0];
        shellCmdParts.erase(shellCmdParts.begin());
        shellCmdParts.push_back(command_);
        auto shell = proc::search_path(std::string{shellCmd});
        if (silently_)
            result = proc::system(shell, proc::args(shellCmdParts), env, proc::start_dir = workingDir_.string(),
                                  proc::std_out > proc::null, proc::std_err > proc::null);
        else
            result = proc::system(shell, proc::args(shellCmdParts), env, proc::start_dir = workingDir_.string(),
                                  proc::std_err > stdout);
    }
    else {
        auto cmdParts = splitCommand(command_);
        auto cmd = cmdParts[0];
        auto cmdArgs = cmdParts | ranges::views::drop(1) | ranges::to<std::vector>;
        if (silently_)
            result = proc::system(std::string{cmd}, proc::args(cmdArgs), env, proc::start_dir = workingDir_.string(),
                                  proc::std_out > proc::null, proc::std_err > proc::null);
        else
            result = proc::system(std::string{cmd}, proc::args(cmdArgs), env, proc::start_dir = workingDir_.string(),
                                  proc::std_err > stdout);
    }

    if (uncheckedResult_)
        return TestActionResult::Success();

    if (result != 0)
        return TestActionResult::Failure(fmt::format("Launched process '{}' returned non-zero exit code", command_));
    else
        return TestActionResult::Success();
}

}