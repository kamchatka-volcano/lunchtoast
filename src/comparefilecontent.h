#pragma once
#include "testactionresult.h"
#include <filesystem>

namespace lunchtoast {

class CompareFileContent {
public:
    CompareFileContent(
            std::filesystem::path filePath,
            std::string expectedFileContent,
            std::filesystem::path workingDir,
            int actionIndex);
    TestActionResult operator()() const;

private:
    std::filesystem::path filePath_;
    std::string expectedFileContent_;
    std::filesystem::path workingDir_;
    int actionIndex_;
};

} //namespace lunchtoast