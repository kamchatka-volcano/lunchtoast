#pragma once
#include "itestaction.h"
#include <filesystem>
#include <string>
#include <optional>

namespace lunchtoast {

class LaunchProcess : public ITestAction {
public:
    LaunchProcess(std::string command,
                  std::filesystem::path workingDir,
                  std::optional<std::string> shellCommand,
                  bool uncheckedResult);
    TestActionResult process() override;
    TestActionType type() const override;

private:
    std::string command_;
    std::filesystem::path workingDir_;
    std::optional<std::string> shellCommand_;
    bool uncheckedResult_;
};

}
