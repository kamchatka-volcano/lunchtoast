#include "comparefilecontent.h"
#include "utils.h"
#include <fmt/format.h>
#include <utility>


namespace lunchtoast {
namespace fs = std::filesystem;

CompareFileContent::CompareFileContent(fs::path filePath,
                                       std::string expectedFileContent,
                                       TestActionType actionType)
        : filePath_(std::move(filePath)), expectedFileContent_(std::move(expectedFileContent)), actionType_(actionType)
{
}

TestActionType CompareFileContent::type() const
{
    return actionType_;
}

TestActionResult CompareFileContent::process()
{
    if (!fs::exists(filePath_))
        return TestActionResult::Failure(
                fmt::format("File content check has failed: {} doesn't exist", filePath_.filename().string()));
    auto result = (readFile(filePath_) == expectedFileContent_);
    if (!result)
        return TestActionResult::Failure(
                fmt::format("File {} content isn't equal to the expected string", filePath_.filename().string()));
    return TestActionResult::Success();
}

}