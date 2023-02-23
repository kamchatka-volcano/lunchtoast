#pragma once
#include <filesystem>
#include <regex>
#include <string>
#include <vector>

namespace lunchtoast {

class FilenameGroup {
public:
    FilenameGroup(std::string filenameOrRegexp, std::filesystem::path directory);
    std::vector<std::filesystem::path> fileList() const;
    std::vector<std::filesystem::path> pathList() const;
    std::string string() const;

private:
    std::string filenameOrRegexp_;
    std::filesystem::path directory_;
    std::regex fileMatchingRegexp_;
    bool isRegexp_;
};

std::vector<FilenameGroup> readFilenames(const std::string& input, const std::filesystem::path& directory);

} //namespace lunchtoast