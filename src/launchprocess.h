#pragma once
#include "itestaction.h"
#include <filesystem>
#include <string>

namespace lunchtoast {

class LaunchProcess : public ITestAction {
public:
    LaunchProcess(std::string command,
                  std::filesystem::path workingDir,
                  std::string shellCommand,
                  bool uncheckedResult,
                  bool silently);
    TestActionResult process() override;
    TestActionType type() const override;

private:
    std::string command_;
    std::filesystem::path workingDir_;
    std::string shellCommand_;
    bool uncheckedResult_;
    bool silently_;
};

}
