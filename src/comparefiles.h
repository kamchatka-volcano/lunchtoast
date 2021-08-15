#pragma once
#include "testaction.h"
#include "alias_filesystem.h"
#include "filenamereader.h"
#include <vector>
#include <regex>

class CompareFiles : public TestAction
{
public:
    CompareFiles(FilenameGroup lhs,
                 FilenameGroup rhs,
                 TestActionType actionType);

    TestActionResult process() override;
    TestActionType type() const override;

private:
    bool compareFiles(const fs::path& lhs, const fs::path& rhs, std::string& failedComparisonInfo) const;

private:
    FilenameGroup lhs_;
    FilenameGroup rhs_;
    TestActionType actionType_;
};

