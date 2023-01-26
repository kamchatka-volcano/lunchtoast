#include "test.h"
#include "testlauncher.h"
#include "testreporter.h"
#include "testcontentsgenerator.h"
#include <fmt/format.h>
#include <cmdlime/commandlinereader.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

struct EnsurePathExists{
    void operator()(const fs::path& path)
    {
        if (!fs::exists(path))
            throw cmdlime::ValidationError{fmt::format("specified test directory "
               "or file path '{}' doesn't exist.\n", path.string())};
    }
};

struct Cfg : public cmdlime::Config{
    CMDLIME_ARG(testPath, fs::path)               << EnsurePathExists{};
    CMDLIME_PARAM(report, fs::path)()             << "save test report to file";
    CMDLIME_PARAM(ext, std::string)(".toast")     << "the extension of searched test files, "
                                                     "required when specified test path is a directory";
    CMDLIME_PARAM(width, int)(48)                 << "set test report's width in number of characters";
    CMDLIME_FLAG(saveContents)                    << "save the current contents of of the test directory";
    CMDLIME_PARAM(shell, std::string)("sh -c -e") << "shell command" << cmdlime::WithoutShortName{};
    CMDLIME_FLAG(noCleanup)                       << "cleanup test files" << cmdlime::WithoutShortName{};
};

int generateTestContens(const Cfg& cfg);
int mainApp(const Cfg& cfg)
{
    if (cfg.saveContents)
        return generateTestContens(cfg);

    auto allTestsPassed = false;
    try{
        const auto testReporter = lunchtoast::TestReporter{cfg.report, cfg.width};
        auto testLauncher = lunchtoast::TestLauncher{cfg.testPath, cfg.ext, cfg.shell, !cfg.noCleanup, testReporter};
        allTestsPassed = testLauncher.process();
    } catch(const std::exception& e){
        fmt::print("Unknown error occurred during test processing: {}\n", e.what());
        return -1;
    }

    if (allTestsPassed)
        return 0;
    else
        return 1;
}

int main(int argc, char **argv)
{
    auto cmdlineReader = cmdlime::CommandLineReader{"lunchtoast"};
    cmdlineReader.setErrorOutputStream(std::cout);
    return cmdlineReader.exec<Cfg>(argc, argv, mainApp);
}

int generateTestContens(const Cfg& cfg)
{
    try{
        auto testContentsGenerator = lunchtoast::TestContentsGenerator{cfg.testPath, cfg.ext};
        if (testContentsGenerator.process())
            return 0;
        else
            return -1;
    }
    catch(const std::exception& e){
        fmt::print("Unknown error occurred during creation of test cleanup whitelist: {}\n", e.what());
        return -1;
    }
}
