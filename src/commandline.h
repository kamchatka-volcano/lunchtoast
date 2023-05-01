#pragma once
#include "utils.h"
#include <cmdlime/config.h>
#include <cmdlime/postprocessor.h>
#include <fmt/format.h>
#include <sfun/path.h>
#include <sfun/utility.h>
#include <gsl/util>
#include <filesystem>
#include <set>
#include <string>

namespace lunchtoast {

struct EnsureDirectoryExists {
    void operator()(const std::filesystem::path& path)
    {
        auto fsError = std::error_code{};
        if (!std::filesystem::exists(path, fsError))
            throw cmdlime::ValidationError{fmt::format(
                    "specified test directory '{}' "
                    "doesn't exist.\n",
                    sfun::path_string(path))};
        if (!std::filesystem::is_directory(path, fsError))
            throw cmdlime::ValidationError{fmt::format(
                    "specified test directory '{}' "
                    "isn't a directory.\n",
                    sfun::path_string(path))};
    }
};

struct EnsureContainsUniqueElements {
    void operator()(const std::vector<std::string>& list)
    {
        auto set = std::set<std::string>{list.begin(), list.end()};
        if (std::ssize(list) != std::ssize(set))
            throw cmdlime::ValidationError{"must contain unique elements"};
    }
};

// clang-format off

struct CommandSaveContents : public cmdlime::Config{
    CMDLIME_ARG(testPath, std::filesystem::path)               << "test directory" << EnsureDirectoryExists{};
};

struct CommandLine : public cmdlime::Config{
    CMDLIME_ARG(testPath, std::filesystem::path)               << "directory containing tests" << EnsureDirectoryExists{};
    CMDLIME_PARAM(config, std::filesystem::path)()             << "config file for setting variables and actions";
    CMDLIME_PARAM(shell, std::string)("bash -ceo pipefail")    << "shell command";
    CMDLIME_PARAM(listFailedTests, std::filesystem::path)()    << "write a list of failed tests to the specified file";
    CMDLIME_PARAM(collectFailedTests, std::filesystem::path)() << "copy directories containing failed tests to the specified path";
    CMDLIME_PARAMLIST(select, std::vector<std::string>)()      << "select tests by tag names" << EnsureContainsUniqueElements{};
    CMDLIME_PARAMLIST(skip, std::vector<std::string>)()        << "skip tests by tag names" << EnsureContainsUniqueElements{};
    CMDLIME_FLAG(withoutCleanup)                               << "disable cleanup of test files";
    CMDLIME_PARAM(reportWidth, int)(48)                        << "set the test report's width as the number of characters";
    CMDLIME_PARAM(reportFile, std::filesystem::path)()         << "write the test report to the specified file";
    CMDLIME_PARAM(searchDepth, cmdlime::optional<int>)         << "the number of descents into child directories levels for tests searching";
    CMDLIME_COMMAND(saveContents, CommandSaveContents)         << "save the current contents of the test directory";
};
// clang-format on

} //namespace lunchtoast

namespace cmdlime {
template<>
struct PostProcessor<lunchtoast::CommandLine> {
    void operator()(lunchtoast::CommandLine& cfg)
    {
        namespace fs = std::filesystem;

        if (cfg.saveContents.has_value()) {
            cfg.saveContents->testPath = fs::absolute(cfg.saveContents->testPath);
            return;
        }

        cfg.testPath = fs::canonical(cfg.testPath);

        const auto path = fs::current_path();
        const auto restorePath = gsl::finally(
                [path]
                {
                    fs::current_path(path);
                });
        fs::current_path(cfg.testPath);

        if (!cfg.config.empty() && cfg.config.is_relative())
            cfg.config = fs::weakly_canonical(cfg.config);

        if (!cfg.reportFile.empty() && cfg.reportFile.is_relative())
            cfg.reportFile = fs::weakly_canonical(cfg.reportFile);

        if (!cfg.listFailedTests.empty() && cfg.listFailedTests.is_relative())
            cfg.listFailedTests = fs::weakly_canonical(cfg.listFailedTests);

        if (!cfg.collectFailedTests.empty() && cfg.collectFailedTests.is_relative())
            cfg.collectFailedTests = fs::weakly_canonical(cfg.collectFailedTests);
    }
};
} //namespace cmdlime