#pragma once
#include "launchprocessresult.h"
#include "processresultcheckmode.h"
#include "testactionresult.h"
#include <boost/process/child.hpp>
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
            int actionIndex,
            std::vector<boost::process::child>* detachedProcessList = nullptr,
            bool skipReadingOutput = false);
    TestActionResult operator()() const;

private:
    std::string command_;
    std::filesystem::path workingDir_;
    std::optional<std::string> shellCommand_;
    std::set<ProcessResultCheckMode> checkModeSet_;
    int actionIndex_;
    std::vector<boost::process::child>* detachedProcessList_;
    bool skipReadingOutput_;
};

} //namespace lunchtoast
