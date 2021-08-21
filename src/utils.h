#pragma once
#include "alias_filesystem.h"
#include <string>
#include <vector>

std::string readFile(const fs::path& filePath);
std::string calcMd5(const fs::path& filePath);
std::string calcMd5(const std::string& data);
void processVariablesSubstitution(std::string& value,
                                  const std::string& varFileName,
                                  const std::string& varDirName);
std::vector<fs::path> getDirectoryContent(const fs::path& dir);

fs::path homePath(const fs::path& path);
std::string homePathString(const fs::path& path);