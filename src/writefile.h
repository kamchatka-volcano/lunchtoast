#pragma once
#include "testactionresult.h"
#include "alias_boost_filesystem.h"
#include <string>

class WriteFile
{
public:
    WriteFile(const fs::path& fileName, const std::string& content);
    TestActionResult process() const;

private:
    fs::path filePath_;
    std::string content_;
};

