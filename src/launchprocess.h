#pragma once
#include "testaction.h"
#include "alias_filesystem.h"
#include <string>

namespace lunchtoast {

class LaunchProcess : public TestAction {
public:
    LaunchProcess(std::string command,
                  fs::path workingDir,
                  std::string shellCommand,
                  bool uncheckedResult,
                  bool silently);
    TestActionResult process() override;
    TestActionType type() const override;

private:
    std::string command_;
    fs::path workingDir_;
    std::string shellCommand_;
    bool uncheckedResult_;
    bool silently_;
};

}
