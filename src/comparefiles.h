#pragma once
#include "testactionresult.h"
#include "alias_boost_filesystem.h"
#include <vector>
#include <regex>

class ComparedFiles{
public:
    ComparedFiles(const fs::path& filePath);
    std::vector<fs::path> pathList() const;
    std::string string() const;
private:
    fs::path filePath_;
    std::regex fileMatchingRegexp_;
    bool isRegexp_ = false;
};

class CompareFiles
{
public:
    CompareFiles(const fs::path& lhs,
                 const fs::path& rhs);

    TestActionResult process() const;

private:
    ComparedFiles lhs_;
    ComparedFiles rhs_;    
};

