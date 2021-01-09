#include "test.h"
#include "testlauncher.h"
#include "testreporter.h"
#include "cleanupwhitelistgenerator.h"
#include "alias_boost_program_options.h"
#include "alias_boost_program_options.h"
#include <spdlog/fmt/fmt.h>

void printUsageInfo()
{
    fmt::print("Usage: lunchtoast [options] [dir|file]path\n"
               "Options:\n"
               " --ext <file extension>   the extension of searched test files,\n"
               "                          required when specified test path is a directory\n"
               "                          (default value: .toast)\n"
               " --report <file path>     save test report to file\n"
               " --reportwidth <number>   set test report's width in number of characters\n"
               " --preserve-current-state generate cleanup whitelist with content\n"
               "                          of the test directory\n"
               " --help                   show usage info\n");
}

struct Cfg{
    fs::path testPath;
    fs::path reportFilePath;
    std::string testFileExtension = ".toast";
    int reportWidth = 48;
    bool generateCleanupWhitelist = false;
};
bool parseCommandLine(Cfg& cfg, int argc, char**argv);
int generateCleanupWhiteList(const Cfg& cfg);

int main(int argc, char **argv)
{
    auto cfg = Cfg{};
    if (!parseCommandLine(cfg, argc, argv)){
        printUsageInfo();
        return -1;
    }

    if (cfg.generateCleanupWhitelist)
        return generateCleanupWhiteList(cfg);

    auto allTestsPassed = false;
    try{
        const auto testReporter = TestReporter{cfg.reportFilePath, cfg.reportWidth};
        auto testLauncher = TestLauncher{cfg.testPath, cfg.testFileExtension, testReporter};
        allTestsPassed = testLauncher.process();
    } catch(const std::exception& e){
        fmt::print("Unknown error occured during test processing: {}\n", e.what());
        return -1;
    }

    if (allTestsPassed)
        return 0;
    else
        return 1;
}

int generateCleanupWhiteList(const Cfg& cfg)
{
    try{
        auto whiteListGenerator = CleanupWhitelistGenerator{cfg.testPath, cfg.testFileExtension};
        if (whiteListGenerator.process())
            return 0;
        else
            return -1;
    }
    catch(const std::exception& e){
        fmt::print("Unknown error occured during creation of test cleanup whitelist: {}\n", e.what());
        return -1;
    }
}

bool parseCommandLine(Cfg& cfg, int argc, char**argv)
{
    auto optionsList = opts::options_description{"options"};
    optionsList.add_options()
        ("help,h", "")
        ("preserve-current-state,p", "")
        ("input",  opts::value(&cfg.testPath))
        ("report,r", opts::value(&cfg.reportFilePath))
        ("reportwidth,w", opts::value(&cfg.reportWidth))
        ("ext,e",    opts::value(&cfg.testFileExtension))
    ;
    auto args = opts::positional_options_description{};
    args.add("input", 1);

    auto params = opts::variables_map{};
    auto parser = opts::command_line_parser{argc, argv}
                  .options(optionsList)
                  .style(opts::command_line_style::default_style & ~opts::command_line_style::allow_guessing)
                  .positional(args);
    try{
        opts::store(parser.run(), params);
        opts::notify(params);
    } catch(const std::exception& e){
        fmt::print("Command line error: {}\n", e.what());
        return false;
    }

    if (params.count("help"))
        return false;
    if (params.count("preserve-current-state"))
        cfg.generateCleanupWhitelist = true;

    if (cfg.testPath.empty()){
        fmt::print("Command line error: the argument with test directory "
                   "or file path is required but missing.\n");
        return false;
    }
    if (!fs::exists(cfg.testPath)){
        fmt::print("Command line error: specified test directory "
                   "or file path '{}' doesn't exist.\n", cfg.testPath.string());
        return false;
    }
    return true;
}
