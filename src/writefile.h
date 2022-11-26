#pragma once
#include "testaction.h"
#include "alias_filesystem.h"
#include <string>


namespace lunchtoast{

class WriteFile : public TestAction{
public:
    WriteFile(fs::path fileName, std::string content);
    TestActionResult process() override;
    TestActionType type() const override;

private:
    fs::path filePath_;
    std::string content_;
};

}

