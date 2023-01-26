#include "testcontentsgenerator.h"
#include "sectionsreader.h"
#include "utils.h"
#include "test.h"
#include "errors.h"
#include <sfun/string_utils.h>
#include <fmt/format.h>
#include <fstream>
#include <sstream>


namespace lunchtoast {
namespace fs = std::filesystem;

namespace {
void processTestConfig(const fs::path& cfgPath);
}

TestContentsGenerator::TestContentsGenerator(const fs::path& testPath,
                                             const std::string& testFileExt)
{
    collectTestConfigs(testPath, testFileExt);
}

void TestContentsGenerator::collectTestConfigs(const fs::path& testPath, const std::string& testFileExt)
{
    if (fs::is_directory(testPath)) {
        if (testFileExt.empty())
            throw std::runtime_error{"To launch all tests in the directory, test extension must be specified"};

        const auto end = fs::directory_iterator{};
        for (auto it = fs::directory_iterator{testPath}; it != end; ++it)
            if (fs::is_directory(it->status()))
                collectTestConfigs(it->path(), testFileExt);
            else if (it->path().extension().string() == testFileExt)
                testConfigs_.push_back(fs::canonical(it->path()));
    } else
        testConfigs_.push_back(fs::canonical(testPath));
}

bool TestContentsGenerator::process()
{
    if (testConfigs_.empty()) {
        fmt::print("No tests were found for generation of test contents. Exiting.");
        return false;
    }

    for (const auto& cfgPath: testConfigs_) {
        try {
            processTestConfig(cfgPath);
        } catch (const TestConfigError& error) {
            fmt::print("Can't generate test contents in config {}. Error: {}\n", homePathString(cfgPath),
                       error.what());
        }
        fmt::print("Generating test contents in test config {}\n", homePathString(cfgPath));
    }
    return true;
}

namespace {
fs::path getTestDirectory(const fs::path& cfgPath, const std::vector<Section>& sections)
{
    auto testDir = cfgPath.parent_path();
    auto cfgDir = fs::path{};
    const auto dirSectionIt = std::find_if(
            sections.begin(),
            sections.end(),
            [](const Section& section)
            {
                return section.name == "Directory";
            });
    if (dirSectionIt != sections.end())
        cfgDir = sfun::trim(dirSectionIt->value);
    if (!cfgDir.empty())
        testDir = fs::canonical(testDir) / cfgDir;
    return testDir;
}

std::string getDirectoryContentString(const fs::path& dir)
{
    auto testDirPaths = getDirectoryContent(dir);
    auto testDirPathsStr = std::vector<std::string>{};
    auto pathRelativeToDir = [&dir](const fs::path& path) { return fs::relative(path, dir).string(); };
    std::transform(testDirPaths.begin(), testDirPaths.end(), std::back_inserter(testDirPaths), pathRelativeToDir);
    std::sort(testDirPathsStr.begin(), testDirPathsStr.end());
    return sfun::join(testDirPathsStr, " ");
}

void writeSections(const std::vector<Section>& sections, const fs::path& outFilePath)
{
    auto stream = std::ofstream{};
    stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    stream.open(outFilePath.string());
    for (const auto& section: sections)
        stream << section.originalText;
}

void copyComments(const std::string& input, std::string& output)
{
    auto stream = std::stringstream{input};
    auto line = std::string{};
    auto sectionEncountered = false;
    while (std::getline(stream, line)){
        if (sfun::trim(line).empty())
            output += "\n";
        else if (sfun::startsWith(line, "#")){
            if (!sectionEncountered)
                output.insert(0, line + "\n");
            else
                output += line + "\n";
        }
        else if (sfun::startsWith(line, "-"))
            sectionEncountered = true;
    }
}

void processTestConfig(const fs::path& cfgPath)
{
    auto stream = std::ifstream{cfgPath.string()};
    if (!stream.is_open())
        throw TestConfigError{fmt::format("Test config file {} doesn't exist", homePathString(cfgPath))};

    auto sections = readSections(stream);
    auto testDir = getTestDirectory(cfgPath, sections);
    const auto testDirContent = getDirectoryContentString(testDir);
    auto newWhiteListSection = Section{"Contents", testDirContent, "-Contents: " + testDirContent + "\n"};

    const auto whitelistSectionIt = std::find_if(sections.cbegin(), sections.cend(), [](const Section& section) {
        return section.name == "Contents";
    });
    if (whitelistSectionIt != sections.cend()) {
        auto whitelistSectionPos = std::distance(sections.cbegin(), whitelistSectionIt);
        copyComments(whitelistSectionIt->originalText, newWhiteListSection.originalText);
        sections.erase(whitelistSectionIt);
        sections.insert(sections.begin() + whitelistSectionPos, newWhiteListSection);
    } else {
        sections.push_back(newWhiteListSection);
    }
    writeSections(sections, cfgPath);
}
}

}
