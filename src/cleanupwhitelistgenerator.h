#pragma once
#include "alias_filesystem.h"
#include <vector>
#include <string>


namespace lunchtoast {

class CleanupWhitelistGenerator {
public:
    CleanupWhitelistGenerator(const fs::path& testPath,
                              const std::string& testFileExt);
    bool process();

private:
    void collectTestConfigs(const fs::path& testPath, const std::string& testFileExt);

private:
    std::vector<fs::path> testConfigs_;
};

}