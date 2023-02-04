#include "writefile.h"
#include <fmt/format.h>
#include <fstream>
#include <utility>


namespace lunchtoast{
namespace fs = std::filesystem;

WriteFile::WriteFile(fs::path filePath, std::string content)
        : filePath_(std::move(filePath)), content_(std::move(content))
{
}

TestActionResult WriteFile::operator()()
{
    auto fileStream = std::ofstream(filePath_.string());
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try{
        fileStream.write(content_.c_str(), static_cast<std::streamsize>(content_.size()));
    } catch (std::exception& e){
        return TestActionResult::Failure(fmt::format("File {} writing error: {}", filePath_.string(), e.what()));
    }
    return TestActionResult::Success();
}

}