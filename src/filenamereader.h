#pragma once
#include "alias_filesystem.h"
#include <regex>
#include <string>
#include <vector>

class FilenameGroup{
public:
    FilenameGroup(std::string filenameOrRegexp, fs::path directory);
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
