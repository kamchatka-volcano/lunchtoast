#include "launchprocess.h"
#include "utils.h"
#include "errors.h"
#include <fmt/format.h>
#include <sfun/string_utils.h>
#include <boost/process.hpp>
#include <filesystem>
#include <utility>

namespace lunchtoast {

namespace proc = boost::process;
namespace fs = std::filesystem;

LaunchProcess::LaunchProcess(
        std::string command,
        fs::path workingDir,
        std::optional<std::string> shellCommand,
        bool uncheckedResult)
    : command_(std::move(command))
    , workingDir_(std::move(workingDir))
    , shellCommand_(std::move(shellCommand))
    , uncheckedResult_(uncheckedResult)
{
    auto paths = boost::this_process::path();
}

TestActionType LaunchProcess::type() const
{
    return TestActionType::RequiredOperation;
}

namespace {

auto pathToString(const std::filesystem::path& path)
{
#ifndef _WIN32
    return path.string();
#else
    return path.wstring();
#endif
}

} //namespace

TestActionResult LaunchProcess::process()
{
    auto cmdParts = shellCommand_.has_value() ? splitCommand(shellCommand_.value()) : splitCommand(command_);
    auto paths = boost::this_process::path();
    paths.emplace_back(workingDir_);
    auto cmd = proc::search_path(std::string{cmdParts[0]}, paths);
    if (cmd.empty())
        throw TestConfigError{fmt::format("Couldn't find the executable of a command '{}'", cmdParts[0])};

    cmdParts.erase(cmdParts.begin());
    if (shellCommand_.has_value())
        cmdParts.push_back(command_);

    auto result = proc::system(
            cmd,
            proc::args(cmdParts),
            proc::start_dir = pathToString(workingDir_),
            proc::std_out > proc::null,
            proc::std_err > proc::null);

    if (uncheckedResult_)
        return TestActionResult::Success();

    if (result != 0)
        return TestActionResult::Failure(fmt::format("Launched process '{}' returned non-zero exit code", command_));
    else
        return TestActionResult::Success();
}

} //namespace lunchtoast