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
#include <set>

using namespace lunchtoast;
namespace fs = std::filesystem;

int generateTestContents(const CommandLine& commandLine);
int mainApp(const CommandLine& commandLine)
{
    if (commandLine.saveContents)
        return generateTestContents(commandLine);

    const auto cfg = [&]
    {
        if (commandLine.config.empty())
            return Config{};
        auto cfgReader = figcone::ConfigReader{};
        return cfgReader.readShoalFile<Config>(commandLine.config);
    }();

    auto allTestsPassed = false;
    try {
        const auto testReporter = TestReporter{commandLine.report, commandLine.width};
        auto testLauncher = TestLauncher{testReporter, commandLine, cfg};
        allTestsPassed = testLauncher.process();
    }
    catch (const std::exception& e) {
        fmt::print("Unknown error occurred during test processing: {}\n", e.what());
        return -1;
    }

    if (allTestsPassed)
        return 0;
    else
        return 1;
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

int generateTestContents(const CommandLine& commandLine)
{
    try {
        auto testContentsGenerator = TestContentsGenerator{commandLine.testPath, commandLine.ext};
        if (testContentsGenerator.process())
            return 0;
        else
            return -1;
    }
    catch (const std::exception& e) {
        fmt::print("Unknown error occurred during creation of test cleanup whitelist: {}\n", e.what());
        return -1;
    }
}
