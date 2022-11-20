#include "test.h"
#include "testlauncher.h"
#include "testreporter.h"
#include "cleanupwhitelistgenerator.h"
#include <fmt/format.h>
#include <cmdlime/commandlinereader.h>

struct EnsurePathExists{
    void operator()(const fs::path& path)
    {
        if (!fs::exists(path))
            throw cmdlime::ValidationError{fmt::format("specified test directory "
               "or file path '{}' doesn't exist.\n", path.string())};
    }
};

struct Cfg : public cmdlime::Config{
    CMDLIME_ARG(testPath, fs::path)             << EnsurePathExists{};
    CMDLIME_PARAM(report, fs::path)()           << "save test report to file";
    CMDLIME_PARAM(ext, std::string)(".toast")   << "the extension of searched test files, "
                                                   "required when specified test path is a directory";
    CMDLIME_PARAM(width, int)(48)               << "set test report's width in number of characters";
    CMDLIME_FLAG(saveState)                     << "generate cleanup whitelist with content\n"
                                                   "of the test directory";
};

int generateCleanupWhiteList(const Cfg& cfg);
int mainApp(const Cfg& cfg)
{
    if (cfg.saveState)
        return generateCleanupWhiteList(cfg);

    auto allTestsPassed = false;
    try{
        const auto testReporter = TestReporter{cfg.report, cfg.width};
        auto testLauncher = TestLauncher{cfg.testPath, cfg.ext, testReporter};
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

int generateCleanupWhiteList(const Cfg& cfg)
{
    try{
        auto whiteListGenerator = CleanupWhitelistGenerator{cfg.testPath, cfg.ext};
        if (whiteListGenerator.process())
            return 0;
        else
            return -1;
    }
    catch(const std::exception& e){
        fmt::print("Unknown error occurred during creation of test cleanup whitelist: {}\n", e.what());
        return -1;
    }
}
