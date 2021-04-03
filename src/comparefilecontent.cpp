#include "comparefilecontent.h"
#include "utils.h"
#include "alias_filesystem.h"
#include <spdlog/fmt/fmt.h>

CompareFileContent::CompareFileContent(const fs::path& filePath,
                                       const std::string& expectedFileContent,
                                       TestActionType actionType)
    : filePath_(filePath)
    , expectedFileContent_(expectedFileContent)
    , actionType_(actionType)
{
}

TestActionType CompareFileContent::type() const
{
    return actionType_;
}

TestActionResult CompareFileContent::process()
{
    if (!fs::exists(filePath_))
        return TestActionResult::Failure(fmt::format("File content check has failed: {} doesn't exist", filePath_.filename().string()));
    auto result = (calcMd5(filePath_) == calcMd5(expectedFileContent_));
    if (!result)
        return TestActionResult::Failure(fmt::format("File {} content isn't equal to the expected string", filePath_.filename().string()));
    return TestActionResult::Success();
}

