#pragma once
#include "alias_boost_filesystem.h"
#include <string>

std::string readFile(const fs::path& filePath);
std::string calcMd5(const fs::path& filePath);
std::string calcMd5(const std::string& data);
void processVariablesSubstitution(std::string& value,
                                  const std::string& varFileName,
                                  const std::string& varDirName);
