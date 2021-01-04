#include "comparefilecontent.h"
#include "utils.h"
#include "alias_boost_filesystem.h"
#include <iostream>

CompareFileContent::CompareFileContent(const fs::path& filePath,
                                       const std::string& expectedFileContent)
    : filePath_(filePath)
    , expectedFileContent_(expectedFileContent)
{
}


TestActionResult CompareFileContent::process() const
{
    if (!fs::exists(filePath_))
        return TestActionResult::Failure("File content check has failed: " + filePath_.filename().string() + " doesn't exist");
    auto result = (calcMd5(filePath_) == calcMd5(expectedFileContent_));
    if (!result)
        return TestActionResult::Failure("File " + filePath_.filename().string() + " content isn't equal to the expected string");
    return TestActionResult::Success();
}

