#pragma once
#include "testactionresult.h"
#include <filesystem>

namespace lunchtoast {

class CompareFileContent {
public:
    CompareFileContent(std::filesystem::path filePath, std::string expectedFileContent);
    TestActionResult operator()() const;

private:
    std::filesystem::path filePath_;
    std::string expectedFileContent_;
};

} //namespace lunchtoast