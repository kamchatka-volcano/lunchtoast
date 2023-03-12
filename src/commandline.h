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

struct EnsurePathExists {
    void operator()(const std::filesystem::path& path)
    {
        auto fsError = std::error_code{};
        if (!std::filesystem::exists(path, fsError))
            throw cmdlime::ValidationError{fmt::format(
                    "specified test directory "
                    "or file path '{}' doesn't exist.\n",
                    sfun::pathString(path))};
    }
};

struct EnsureContainsUniqueElements {
    void operator()(const std::vector<std::string>& list)
    {
        auto set = std::set<std::string>{list.begin(), list.end()};
        if (sfun::ssize(list) != sfun::ssize(set))
            throw cmdlime::ValidationError{"must contain unique elements"};
    }
};

// clang-format off
struct CommandLine : public cmdlime::Config{
    CMDLIME_ARG(testPath, std::filesystem::path)               << "a test file or a directory containing tests" << EnsurePathExists{};
    CMDLIME_PARAM(config, std::filesystem::path)()             << "a config file for setting variables";
    CMDLIME_PARAM(report, std::filesystem::path)()             << "write a test report to the specified file";
    CMDLIME_PARAM(listFailedTests, std::filesystem::path)()    << "write a list of failed tests to the specified file";
    CMDLIME_PARAM(collectFailedTests, std::filesystem::path)() << "copy directories containing failed tests to the specified path";
    CMDLIME_PARAM(ext, std::string)(".toast")                  << "the extension of searched test files, "
                                                                  "required when specified test path is a directory";
    CMDLIME_PARAM(width, int)(48)                              << "set test report's width as a number of characters";
    CMDLIME_FLAG(saveContents)                                 << "save the current contents of the test directory";
    CMDLIME_PARAM(shell, std::string)("sh -c -e")              << "shell command";
    CMDLIME_FLAG(noCleanup)                                    << "disables cleanup of test files";
    CMDLIME_PARAMLIST(select, std::vector<std::string>)()      << "select tests by tag names" << EnsureContainsUniqueElements{};
    CMDLIME_PARAMLIST(skip, std::vector<std::string>)()        << "skip tests by tag names" << EnsureContainsUniqueElements{};
};

// clang-format on
} //namespace lunchtoast

namespace cmdlime {
template<>
struct PostProcessor<lunchtoast::CommandLine> {
    void operator()(lunchtoast::CommandLine& cfg)
    {
        namespace fs = std::filesystem;
        auto fsError = std::error_code{};

        const auto baseDir = [&]
        {
            if (fs::is_directory(cfg.testPath, fsError))
                return cfg.testPath;
            else
                return cfg.testPath.parent_path();
        }();

        auto path = fs::current_path(fsError);
        const auto restorePath = gsl::finally(
                [path]
                {
                    fs::current_path(path);
                });
        fs::current_path(baseDir, fsError);

        if (cfg.config.is_relative())
            cfg.config = fs::weakly_canonical(cfg.config, fsError);

        if (cfg.report.is_relative())
            cfg.report = fs::weakly_canonical(cfg.report, fsError);

        if (cfg.listFailedTests.is_relative())
            cfg.listFailedTests = fs::weakly_canonical(cfg.listFailedTests, fsError);

        if (cfg.collectFailedTests.is_relative())
            cfg.collectFailedTests = fs::weakly_canonical(cfg.collectFailedTests, fsError);
    }
};
} //namespace cmdlime