#pragma once
#include "alias_boost_filesystem.h"
#include <regex>
#include <string>
#include <vector>

class FilenameGroup{
public:
    FilenameGroup(const std::string& filenameOrRegexp, const fs::path& directory);
    std::vector<fs::path> fileList() const;
    std::vector<fs::path> pathList() const;
    std::string string() const;
private:
    std::string filenameOrRegexp_;
    fs::path directory_;
    std::regex fileMatchingRegexp_;
    bool isRegexp_;
};

std::vector<FilenameGroup> readFilenames(const std::string& input, const fs::path& directory);
