#include "writefile.h"
#include <fmt/format.h>
#include <sfun/path.h>
#include <sfun/utility.h>
#include <fstream>
#include <utility>

namespace lunchtoast {
namespace fs = std::filesystem;

WriteFile::WriteFile(fs::path filePath, std::string content)
    : filePath_(std::move(filePath))
    , content_(std::move(content))
{
}

TestActionResult WriteFile::operator()() const
{
    auto fileStream = std::ofstream(filePath_, std::ios::binary);
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        fileStream.write(content_.c_str(), std::ssize(content_));
    }
    catch (std::exception& e) {
        return TestActionResult::Failure(
                fmt::format("File {} writing error: {}", sfun::path_string(filePath_), e.what()));
    }
    return TestActionResult::Success();
}

} //namespace lunchtoast