#pragma once
#include "testactionresult.h"
#include "alias_boost_filesystem.h"
#include <string>

class LaunchProcess
{
public:
    LaunchProcess(const std::string& command,
                  const fs::path& workingDir,
                  const std::string& shellCommand,
                  bool uncheckedResult,
                  bool silently);
    TestActionResult process() const;

private:
    bool launchCommand();
    bool launchShellCommand();

private:
    std::string command_;
    fs::path workingDir_;
    std::string shellCommand_;
    bool uncheckedResult_;
    bool silently_;
};

