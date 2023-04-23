#pragma once
#include "testactionresult.h"
#include <filesystem>
#include <string>

namespace lunchtoast {

class WriteFile {
public:
    WriteFile(std::filesystem::path fileName, std::string content);
    TestActionResult operator()() const;

private:
    std::filesystem::path filePath_;
    std::string content_;
};

} //namespace lunchtoast
