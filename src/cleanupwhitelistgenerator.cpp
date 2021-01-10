#include "cleanupwhitelistgenerator.h"
#include "sectionsreader.h"
#include "utils.h"
#include "test.h"
#include <spdlog/fmt/fmt.h>
#include <boost/algorithm/string.hpp>
#include <fstream>

CleanupWhitelistGenerator::CleanupWhitelistGenerator(const fs::path& testPath,
                                                     const std::string& testFileExt)
{
    collectTestConfigs(testPath, testFileExt);
}

void CleanupWhitelistGenerator::collectTestConfigs(const fs::path& testPath, const std::string& testFileExt)
{
    if (fs::is_directory(testPath)){
        if (testFileExt.empty())
            throw std::runtime_error{"To launch all tests in the directory, test extension must be specified"};

        const auto end = fs::directory_iterator{};
        for (auto it = fs::directory_iterator{testPath}; it != end; ++it)
            if (fs::is_directory(it->status()))
                collectTestConfigs(it->path(), testFileExt);
            else if(it->path().extension().string() == testFileExt)
                testConfigs_.push_back(fs::canonical(it->path()));
    }
    else
        testConfigs_.push_back(fs::canonical(testPath));
}

bool CleanupWhitelistGenerator::process()
{    
    if (testConfigs_.empty()){
        fmt::print("No tests were found for generation of cleanup whitelist. Exiting.");
        return false;
    }

    for (const auto& cfgPath : testConfigs_){
        try{
            processTestConfig(cfgPath);
        } catch(const TestConfigError& error){
            fmt::print("Can't generate cleanup whitelist in config {}. Error: {}\n", cfgPath.string(), error.what());
        }
        fmt::print("Generating cleanup whitelist in test config {}\n", cfgPath.string());
    }
    return true;
}

namespace{
    fs::path getTestDirectory(const fs::path &cfgPath, const std::vector<RestorableSection>& sections)
    {
        auto testDir = cfgPath.parent_path();
        auto cfgDir = fs::path{};
        const auto dirSectionIt = std::find_if(sections.cbegin(), sections.cend(), [](const Section& section){return section.name == "Directory";});
        if (dirSectionIt != sections.end())
            cfgDir = dirSectionIt->value;
        if (!cfgDir.empty())
            testDir = fs::canonical(cfgDir, testDir);
        return testDir;
    }

    std::string getDirectoryContentString(const fs::path& dir)
    {
        auto testDirPaths = getDirectoryContent(dir);
        auto testDirPathsStr = std::vector<std::string>{};
        std::transform(testDirPaths.begin(), testDirPaths.end(),
                       std::back_inserter(testDirPathsStr),
                       [&dir](const fs::path& path) {return fs::relative(path, dir).string();});
        return boost::join(testDirPathsStr, " ");
    }

    void writeSections(const std::vector<RestorableSection>& sections, const fs::path& outFilePath)
    {
        auto stream = std::ofstream{};
        stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        stream.open(outFilePath.string());
        for (const auto& section : sections)
            stream << section.originalText;
    }
}

void CleanupWhitelistGenerator::processTestConfig(const fs::path &cfgPath)
{
    auto stream = std::ifstream{cfgPath.string()};
    if (!stream.is_open())
        throw TestConfigError{fmt::format("Test config file {} doesn't exist", cfgPath.string())};

    auto sections = readRestorableSections(stream, {RawSectionSpecifier{"Write", "---"},
                                                    RawSectionSpecifier{"Assert content of", "---"},
                                                    RawSectionSpecifier{"Expect content of", "---"}});
    auto testDir = getTestDirectory(cfgPath, sections);
    const auto testDirContent = getDirectoryContentString(testDir);
    auto newWhiteListSection = RestorableSection{"Cleanup whitelist", testDirContent, "-Cleanup whitelist: " + testDirContent + "\n"};

    const auto whitelistSectionIt = std::find_if(sections.cbegin(), sections.cend(), [](const Section& section){return section.name == "Cleanup whitelist";});
    if (whitelistSectionIt != sections.cend()){
        auto whitelistSectionPos = std::distance(sections.cbegin(), whitelistSectionIt);
        sections.erase(whitelistSectionIt);
        sections.insert(sections.begin() + whitelistSectionPos, newWhiteListSection);
    }
    else{
        sections.push_back(newWhiteListSection);
    }
    writeSections(sections, cfgPath);
}
