#include "launchprocess.h"
#include "alias_boost_process.h"
#include <sfun/string_utils.h>
#include <fmt/format.h>
#include <utility>

namespace proc = boost::process;
namespace str = sfun::string_utils;

LaunchProcess::LaunchProcess(std::string command,
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
    if (!shellCommand_.empty()){
        auto cmdParts = str::split(shellCommand_);
        auto shellCmd = cmdParts[0];
        cmdParts.erase(cmdParts.begin());
        cmdParts.push_back(command_);
        auto shell = proc::search_path(std::string{shellCmd});
        if (silently_)
            result = proc::system(shell, proc::args(cmdParts), env, proc::start_dir = workingDir_.string(), proc::std_out > proc::null, proc::std_err > proc::null);
        else
            result = proc::system(shell, proc::args(cmdParts), env, proc::start_dir = workingDir_.string(), proc::std_err > stdout);
    }
    else{
        if (silently_)
            result = proc::system(proc::cmd(command_), env, proc::start_dir = workingDir_.string(), proc::std_out > proc::null, proc::std_err > proc::null);
        else
            result = proc::system(proc::cmd(command_), env, proc::start_dir = workingDir_.string(), proc::std_err > stdout);
    }

    if (uncheckedResult_)
        return TestActionResult::Success();

    if (result != 0)
        return TestActionResult::Failure(fmt::format("Launched process '{}' returned non-zero exit code", command_));
    else
        return TestActionResult::Success();
}
