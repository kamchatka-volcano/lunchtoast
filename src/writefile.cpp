#include "writefile.h"
#include <spdlog/fmt/fmt.h>
#include <fstream>

WriteFile::WriteFile(const fs::path& filePath, const std::string& content)
    : filePath_(filePath)
    , content_(content)
{
}

TestActionType WriteFile::type() const
{
    return TestActionType::RequiredOperation;
}

TestActionResult WriteFile::process()
{
    auto fileStream = std::ofstream(filePath_.string());
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try{
        fileStream.write(content_.c_str(), static_cast<std::streamsize>(content_.size()));
    } catch(std::exception& e)
    {
        return TestActionResult::Failure(fmt::format("File {} writing error: {}", filePath_.string(), e.what()));
    }
    return TestActionResult::Success();
}
