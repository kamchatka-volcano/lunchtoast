#pragma once
#include <filesystem>
#include <vector>
#include <string>


namespace lunchtoast {

class CleanupWhitelistGenerator {
public:
    CleanupWhitelistGenerator(const std::filesystem::path& testPath,
                              const std::string& testFileExt);
    bool process();

private:
    void collectTestConfigs(const std::filesystem::path& testPath, const std::string& testFileExt);

private:
    std::vector<std::filesystem::path> testConfigs_;
};

}