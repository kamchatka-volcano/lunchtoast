#pragma once
#include "launchprocessresult.h"
#include "processresultcheckmode.h"
#include "testactionresult.h"
#include <gsl/pointers>
#include <filesystem>
#include <optional>
#include <set>
#include <string>

namespace lunchtoast {

class TestAction;

class LaunchProcess {
public:
    LaunchProcess(
            std::string command,
            std::filesystem::path workingDir,
            std::optional<std::string> shellCommand,
            std::set<ProcessResultCheckMode> checkModeSet,
            int actionIndex);
    TestActionResult operator()();
    LaunchProcessResult result() const;

private:
    std::string command_;
    std::filesystem::path workingDir_;
    std::optional<std::string> shellCommand_;
    std::set<ProcessResultCheckMode> checkModeSet_;
    LaunchProcessResult result_;
    int actionIndex_;
};

} //namespace lunchtoast
