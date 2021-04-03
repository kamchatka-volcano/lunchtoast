#pragma once
#include "testaction.h"
#include "alias_filesystem.h"
#include <string>

class LaunchProcess : public TestAction
{
public:
    LaunchProcess(const std::string& command,
                  const fs::path& workingDir,
                  const std::string& shellCommand,
                  bool uncheckedResult,
                  bool silently);
    TestActionResult process() override;
    TestActionType type() const override;

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

