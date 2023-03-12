#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace lunchtoast {

std::string readTextFile(const std::filesystem::path& filePath);
std::string readFile(const std::filesystem::path& filePath);
std::string processVariablesSubstitution(std::string value, const std::unordered_map<std::string, std::string>& vars);
std::vector<std::filesystem::path> getDirectoryContent(const std::filesystem::path& dir);
std::string homePathString(const std::filesystem::path& path);
std::string toLower(std::string_view str);
std::vector<std::string_view> splitCommand(std::string_view str);

} //namespace lunchtoast
