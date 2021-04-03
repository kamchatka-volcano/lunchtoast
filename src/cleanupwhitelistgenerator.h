#pragma once
#include "alias_filesystem.h"
#include <vector>
#include <string>

class CleanupWhitelistGenerator
{
public:
    CleanupWhitelistGenerator(const fs::path& testPath,
                              const std::string& testFileExt);
    bool process();
    void processTestConfig(const fs::path& cfgPath);

private:
    void collectTestConfigs(const fs::path &testPath, const std::string& testFileExt);

private:
    std::vector<fs::path> testConfigs_;
};

