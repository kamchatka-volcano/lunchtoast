#include "launchprocess.h"
#include "alias_boost_process.h"
#include "string_utils.h"
#include <spdlog/fmt/fmt.h>

namespace proc = boost::process;

LaunchProcess::LaunchProcess(const std::string& command,
                             const fs::path& workingDir,
                             const std::string& shellCommand,
                             bool uncheckedResult,
                             bool silently)
    : command_(command)
    , workingDir_(workingDir)
    , shellCommand_(shellCommand)
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
        auto cmdParts = str::splitted(shellCommand_);
        auto shellCmd = cmdParts[0];
        cmdParts.erase(cmdParts.begin());
        cmdParts.push_back(command_);
        auto shell = proc::search_path(shellCmd);
        if (silently_)
            result = proc::system(shell, proc::args(cmdParts), env, proc::start_dir = workingDir_.string(), proc::std_out > proc::null, proc::std_err > proc::null);
        else
            result = proc::system(shell, proc::args(cmdParts), env, proc::start_dir = workingDir_.string());
    }
    else{
        if (silently_)
            result = proc::system(proc::cmd(command_), env, proc::start_dir = workingDir_.string(), proc::std_out > proc::null, proc::std_err > proc::null);
        else
            result = proc::system(proc::cmd(command_), env, proc::start_dir = workingDir_.string());
    }

    if (uncheckedResult_)
        return TestActionResult::Success();

    if (result != 0)
        return TestActionResult::Failure(fmt::format("Launched process '{}' returned non-zero exit code", command_));
    else
        return TestActionResult::Success();
}
