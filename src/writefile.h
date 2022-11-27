#pragma once
#include "testaction.h"
#include <filesystem>
#include <string>


namespace lunchtoast{

class WriteFile : public TestAction{
public:
    WriteFile(std::filesystem::path fileName, std::string content);
    TestActionResult process() override;
    TestActionType type() const override;

private:
    std::filesystem::path filePath_;
    std::string content_;
};

}

