#include "commandline.h"
#include "config.h"
#include "test.h"
#include "testcontentsgenerator.h"
#include "testlauncher.h"
#include "testreporter.h"
#include <cmdlime/commandlinereader.h>
#include <figcone/configreader.h>
#include <fmt/format.h>
#include <filesystem>
#include <optional>

using namespace lunchtoast;
namespace fs = std::filesystem;

int generateTestContents(const CommandSaveContents& commandCfg);
Config readConfig(const CommandLine& commandLine);

int mainApp(const CommandLine& commandLine)
{
    if (commandLine.saveContents.has_value())
        return generateTestContents(commandLine.saveContents.value());

    const auto cfg = readConfig(commandLine);
    try {
        const auto testReporter = TestReporter{commandLine.reportFile, commandLine.reportWidth};
        auto testLauncher = TestLauncher{testReporter, commandLine, cfg};
        const auto allTestPassed = testLauncher.process();
        return allTestPassed ? 0 : 1;
    }
    catch (const std::runtime_error& e) {
        fmt::print(fmt::runtime(e.what()));
        return 2;
    }
    catch (const std::exception& e) {
        fmt::print("Unknown error occurred during test processing: {}\n", e.what());
        return 2;
    }
}

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
{
    auto cmdlineReader = cmdlime::CommandLineReader<cmdlime::Format::Simple>{"lunchtoast"};
    cmdlineReader.setErrorOutputStream(std::cout);
    return cmdlineReader.exec<CommandLine>(argc, argv, mainApp);
}
#else
int main(int argc, char** argv)
{
    auto cmdlineReader = cmdlime::CommandLineReader<cmdlime::Format::Simple>{"lunchtoast"};
    cmdlineReader.setErrorOutputStream(std::cout);
    return cmdlineReader.exec<CommandLine>(argc, argv, mainApp);
}
#endif

int generateTestContents(const CommandSaveContents& commandCfg)
{
    try {
        if (saveTestContents(commandCfg.testPath))
            return 0;
        else
            return 1;
    }
    catch (const std::exception& e) {
        fmt::print("Unknown error occurred during creation of test cleanup whitelist: {}\n", e.what());
        return 2;
    }
}

Config readConfig(const CommandLine& commandLine)
{
    if (!commandLine.config.empty()) {
        auto cfgReader = figcone::ConfigReader{};
        return cfgReader.readShoalFile<Config>(commandLine.config);
    }
    return Config{};
}