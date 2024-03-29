#include "testcontentsgenerator.h"
#include "constants.h"
#include "errors.h"
#include "linestream.h"
#include "sectionsreader.h"
#include "test.h"
#include "utils.h"
#include <fmt/format.h>
#include <range/v3/action/sort.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <sfun/contract.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <fstream>
#include <sstream>

#include <iostream>

namespace lunchtoast {
namespace actions = ranges::actions;
namespace views = ranges::views;
namespace fs = std::filesystem;

namespace {
void processTestConfig(const fs::path& cfgPath);
}

bool saveTestContents(const std::filesystem::path& testPath)
{
    sfun_precondition(is_directory(testPath));

    const auto testCfgPath = fs::canonical(testPath) / hardcoded::testCaseFilename;
    try {
        processTestConfig(testCfgPath);
    }
    catch (const TestConfigError& error) {
        fmt::print("Can't generate test contents in config {}. Error: {}\n", homePathString(testCfgPath), error.what());
        return false;
    }
    fmt::print("Generating test contents in test config {}\n", homePathString(testCfgPath));
    return true;
}

namespace {
fs::path getTestDirectory(const fs::path& cfgPath, const std::vector<Section>& sections)
{
    const auto dirSectionIt = std::ranges::find_if(
            sections,
            [](const Section& section)
            {
                return section.name == "Directory";
            });
    auto testDir = cfgPath.parent_path();
    if (dirSectionIt != sections.end())
        return fs::canonical(testDir) / sfun::make_path(sfun::trim(dirSectionIt->value));
    return testDir;
}

std::string getDirectoryContentString(const fs::path& dir)
{
    const auto testDirPaths = getDirectoryContent(dir);
    const auto pathRelativeToDir = [&dir](const fs::path& path)
    {
        return sfun::path_string(fs::relative(path, dir));
    };
    const auto testDirPathsStr = testDirPaths | //
            views::transform(pathRelativeToDir) | //
            ranges::to<std::vector> | actions::sort;
    return testDirPathsStr | views::join(' ') | ranges::to<std::string>;
}

void writeSections(const std::vector<Section>& sections, const fs::path& outFilePath)
{
    auto stream = std::ofstream{outFilePath, std::ios::binary};
    stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    for (const auto& section : sections)
        stream << section.originalText;
}

void copyComments(const std::string& input, std::string& output)
{
    auto stream = std::stringstream{input};
    auto line = std::string{};
    auto sectionEncountered = false;
    auto lineStream = LineStream{stream};
    line = lineStream.readLine();
    while (!line.empty()) {
        if (sfun::trim(line).empty())
            output += "\n";
        else if (line.starts_with("#")) {
            if (!sectionEncountered)
                output.insert(0, line);
            else
                output += line;
        }
        else if (line.starts_with("-"))
            sectionEncountered = true;

        line = lineStream.readLine();
    }
}

void processTestConfig(const fs::path& cfgPath)
{
    auto stream = std::ifstream{cfgPath, std::ios::binary};
    if (!stream.is_open())
        throw TestConfigError{fmt::format("Test config file {} doesn't exist", homePathString(cfgPath))};

    auto sections = readSections(stream);
    const auto testDir = getTestDirectory(cfgPath, sections);
    const auto testDirContent = getDirectoryContentString(testDir);
    auto newWhiteListSection = Section{"Contents", testDirContent, "-Contents: " + testDirContent + "\n"};

    const auto whitelistSectionIt = std::ranges::find_if(
            sections,
            [](const Section& section)
            {
                return section.name == "Contents";
            });
    if (whitelistSectionIt != sections.end()) {
        auto whitelistSectionPos = std::distance(sections.begin(), whitelistSectionIt);
        copyComments(whitelistSectionIt->originalText, newWhiteListSection.originalText);
        sections.erase(whitelistSectionIt);
        sections.insert(sections.begin() + whitelistSectionPos, newWhiteListSection);
    }
    else
        sections.push_back(newWhiteListSection);

    writeSections(sections, cfgPath);
}
} //namespace

} //namespace lunchtoast
