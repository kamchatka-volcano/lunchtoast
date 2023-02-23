#pragma once
#include "filenamereader.h"
#include "testactionresult.h"
#include <filesystem>
#include <regex>
#include <vector>

namespace lunchtoast {

enum class ComparisonMode {
    Text,
    Binary
};

class CompareFiles {
public:
    CompareFiles(FilenameGroup lhs, FilenameGroup rhs, ComparisonMode mode);
    TestActionResult operator()();

private:
    bool compareFiles(
            const std::filesystem::path& lhs,
            const std::filesystem::path& rhs,
            std::string& failedComparisonInfo) const;

private:
    FilenameGroup lhs_;
    FilenameGroup rhs_;
    ComparisonMode mode_;
};

} //namespace lunchtoast