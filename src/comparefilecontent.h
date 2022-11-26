#pragma once
#include "alias_filesystem.h"
#include "testaction.h"


namespace lunchtoast {

class CompareFileContent : public TestAction {
public:
    CompareFileContent(fs::path filePath,
                       std::string expectedFileContent,
                       TestActionType actionType);
    TestActionResult process() override;
    TestActionType type() const override;

private:
    fs::path filePath_;
    std::string expectedFileContent_;
    TestActionType actionType_;
};

}