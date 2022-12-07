#pragma once
#include "itestaction.h"
#include "filenamereader.h"
#include <filesystem>
#include <vector>
#include <regex>

namespace lunchtoast {

class CompareFiles : public ITestAction {
public:
    CompareFiles(FilenameGroup lhs, FilenameGroup rhs, TestActionType actionType);
    TestActionResult process() override;
    TestActionType type() const override;

private:
    bool compareFiles(
            const std::filesystem::path& lhs,
            const std::filesystem::path& rhs,
            std::string& failedComparisonInfo) const;

private:
    FilenameGroup lhs_;
    FilenameGroup rhs_;
    TestActionType actionType_;
};

}