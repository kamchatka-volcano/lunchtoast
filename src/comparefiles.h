#pragma once
#include "testactionresult.h"
#include "alias_boost_filesystem.h"
#include "filenamereader.h"
#include <vector>
#include <regex>

class CompareFiles
{
public:
    CompareFiles(const FilenameGroup& lhs,
                 const FilenameGroup& rhs);

    TestActionResult process() const;

private:
    bool compareFiles(const fs::path& lhs, const fs::path& rhs, std::string& failedComparisonInfo) const;

private:
    FilenameGroup lhs_;
    FilenameGroup rhs_;
};

