#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace lunchtoast {

class TestContentsGenerator {
public:
    TestContentsGenerator(const std::filesystem::path& testPath, const std::string& testFileExt);
    bool process() const;

private:
    void collectTestConfigs(const std::filesystem::path& testPath, const std::string& testFileExt);

private:
    std::vector<std::filesystem::path> testConfigs_;
};

} //namespace lunchtoast