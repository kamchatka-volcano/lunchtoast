#include "comparefilecontent.h"
#include "utils.h"
#include <fmt/format.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <utility>

namespace lunchtoast {
namespace fs = std::filesystem;

CompareFileContent::CompareFileContent(fs::path filePath, std::string expectedFileContent)
    : filePath_{std::move(filePath)}
    , expectedFileContent_{std::move(expectedFileContent)}
{
    expectedFileContent_ = sfun::replace(expectedFileContent_, "\r\n", "\n");
    expectedFileContent_ = sfun::replace(expectedFileContent_, "\r", "\n");
}

TestActionResult CompareFileContent::operator()()
{
    if (!fs::exists(filePath_))
        return TestActionResult::Failure(
                fmt::format("File content check has failed: {} doesn't exist", sfun::pathString(filePath_.filename())));

    const auto fileContent = readTextFile(filePath_);
    if (fileContent != expectedFileContent_)
        return TestActionResult::Failure(fmt::format(
                "File {} content isn't equal to the expected string",
                sfun::pathString(filePath_.filename())));

    return TestActionResult::Success();
}

} //namespace lunchtoast
