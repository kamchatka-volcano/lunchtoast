#include "launchprocess.h"
#include "alias_boost_process.h"
#include <string_utils.h>
#include <iostream>

namespace proc = boost::process;

LaunchProcess::LaunchProcess(const std::string& command, const fs::path& workingDir, const std::string& shellCommand)
    : command_(command)
    , workingDir_(workingDir)
    , shellCommand_(shellCommand)
{
}

TestActionResult LaunchProcess::process() const
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
        result = proc::system(shell, proc::args(cmdParts), env, proc::start_dir = workingDir_);
    }
    else{
        result = proc::system(proc::cmd(command_), env, proc::start_dir = workingDir_);
    }

    if (result != 0)
        return TestActionResult::Failure("Launched process '" + command_+ "' returned non-zero exit code");
    else
        return TestActionResult::Success();
}
