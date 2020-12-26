#pragma once
#include "alias_boost_filesystem.h"
#include "testactionresult.h"

class CompareFileContent
{
public:
    CompareFileContent(const fs::path& filePath,
                       const std::string& expectedFileContent);
    TestActionResult process() const;

private:
    fs::path filePath_;
    std::string expectedFileContent_;    
};

