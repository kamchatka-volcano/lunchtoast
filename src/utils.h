#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace lunchtoast{

std::string readFile(const std::filesystem::path& filePath);
void processVariablesSubstitution(std::string& value,
                                  const std::string& varFileName,
                                  const std::string& varDirName);
std::vector<std::filesystem::path> getDirectoryContent(const std::filesystem::path& dir);

std::filesystem::path homePath(const std::filesystem::path& path);
std::string homePathString(const std::filesystem::path& path);

std::string withoutLastNewLine(std::string value);
std::string toLower(std::string_view str);

std::vector<std::string_view> splitCommand(std::string_view str);

}
