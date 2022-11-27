#pragma once
#include "testaction.h"
#include <filesystem>


namespace lunchtoast {

class CompareFileContent : public TestAction {
public:
    CompareFileContent(std::filesystem::path filePath,
                       std::string expectedFileContent,
                       TestActionType actionType);
    TestActionResult process() override;
    TestActionType type() const override;

private:
    std::filesystem::path filePath_;
    std::string expectedFileContent_;
    TestActionType actionType_;
};

}