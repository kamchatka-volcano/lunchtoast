#pragma once
#include "alias_filesystem.h"
#include "testaction.h"

class CompareFileContent : public TestAction
{
public:
    CompareFileContent(const fs::path& filePath,
                       const std::string& expectedFileContent,
                       TestActionType actionType);
    TestActionResult process() override;
    TestActionType type() const override;

private:
    fs::path filePath_;
    std::string expectedFileContent_;
    TestActionType actionType_;
};

