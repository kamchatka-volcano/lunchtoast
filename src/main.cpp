#include "test.h"
#include "testlauncher.h"
#include "testreporter.h"
#include "cleanupwhitelistgenerator.h"
#include <spdlog/fmt/fmt.h>
#include <structopt/app.hpp>

struct Cfg{
    fs::path test_path;
    std::optional<fs::path> report = fs::path{};
    std::optional<std::string> ext = ".toast";
    std::optional<int> width = 48;
    std::optional<bool> save_state = false;
};
STRUCTOPT(Cfg, test_path, report, ext, width, save_state);

bool parseCommandLine(Cfg& cfg, int argc, char** argv);
int generateCleanupWhiteList(const Cfg& cfg);

int main(int argc, char **argv)
{
    auto cfg = Cfg{};
    if (!parseCommandLine(cfg, argc, argv))
        return -1;

    if (cfg.save_state.value())
        return generateCleanupWhiteList(cfg);

    auto allTestsPassed = false;
    try{
        const auto testReporter = TestReporter{cfg.report.value(), cfg.width.value()};
        auto testLauncher = TestLauncher{cfg.test_path, cfg.ext.value(), testReporter};
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
        auto whiteListGenerator = CleanupWhitelistGenerator{cfg.test_path, cfg.ext.value()};
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

bool parseCommandLine(Cfg& cfg, int argc, char** argv)
{
    auto parser = structopt::app{"lunchtoast", {},
                                 "Usage: lunchtoast [options] [dir|file]test_path\n"
                                 "Options:\n"
                                 " --ext <file extension>  the extension of searched test files,\n"
                                 "                         required when specified test path is a directory\n"
                                 "                         (default value: .toast)\n"
                                 " --report <file path>    save test report to file\n"
                                 " --width <number>        set test report's width in number of characters\n"
                                 " --save-state            generate cleanup whitelist with content\n"
                                 "                         of the test directory\n"
                                 " --help                  show usage info\n"};
    try{
        cfg = parser.parse<Cfg>(argc, argv);
    }
    catch (const structopt::exception& e) {
        fmt::print("{}\n",e.what());
        fmt::print(e.help());
        return false;
    }
    if (!fs::exists(cfg.test_path)){
        fmt::print("Error: specified test directory "
                   "or file path '{}' doesn't exist.\n", cfg.test_path.string());
        fmt::print(parser.help());
        return false;
    }
    return true;
}
