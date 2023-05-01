#pragma once
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
    CompareFiles(std::filesystem::path lhs, std::filesystem::path rhs, ComparisonMode mode);
    TestActionResult operator()() const;

private:
    std::filesystem::path lhs_;
    std::filesystem::path rhs_;
    ComparisonMode mode_;
};

} //namespace lunchtoast