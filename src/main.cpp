#include "commandline.h"
#include "test.h"
#include "testcontentsgenerator.h"
#include "testlauncher.h"
#include "testreporter.h"
#include <cmdlime/commandlinereader.h>
#include <fmt/format.h>
#include <sfun/utfconv.h>
#include <set>

using namespace lunchtoast;

int generateTestContents(const CommandLine& commandLine);
int mainApp(const CommandLine& commandLine)
{
    if (commandLine.saveContents)
        return generateTestContents(commandLine);

    auto allTestsPassed = false;
    try {
        const auto testReporter = TestReporter{commandLine.report, commandLine.width};
        auto testLauncher = TestLauncher{testReporter, commandLine};
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

#ifndef _WIN32
int main(int argc, char** argv)
{
    auto cmdlineReader = cmdlime::CommandLineReader<cmdlime::Format::Simple>{"lunchtoast"};
    cmdlineReader.setErrorOutputStream(std::cout);
    return cmdlineReader.exec<CommandLine>(argc, argv, mainApp);
}
#else
int wmain(int argc, wchar_t** argv)
{
    auto wargs = std::vector<std::wstring>{argv, argv + argc};
    wargs.erase(wargs.begin());
    auto args = std::vector<std::string>{};
    std::transform(
            wargs.begin(),
            wargs.end(),
            std::back_inserter(args),
            [](const std::wstring& arg)
            {
                return sfun::toUtf8(arg);
            });

    auto cmdlineReader = cmdlime::CommandLineReader<cmdlime::Format::Simple>{"lunchtoast"};
    cmdlineReader.setErrorOutputStream(std::cout);
    return cmdlineReader.exec<CommandLine>(args, mainApp);
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
