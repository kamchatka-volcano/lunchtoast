#pragma once
#include "itestaction.h"
#include <filesystem>


namespace lunchtoast {

class CompareFileContent : public ITestAction {
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