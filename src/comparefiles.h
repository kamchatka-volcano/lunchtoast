#pragma once
#include "filenamereader.h"
#include "testactionresult.h"
#include <filesystem>
#include <vector>
#include <regex>

namespace lunchtoast {

class CompareFiles{
public:
    CompareFiles(FilenameGroup lhs, FilenameGroup rhs);
    TestActionResult operator()();

private:
    bool compareFiles(
            const std::filesystem::path& lhs,
            const std::filesystem::path& rhs,
            std::string& failedComparisonInfo) const;

private:
    FilenameGroup lhs_;
    FilenameGroup rhs_;
};

}