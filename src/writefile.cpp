#include "writefile.h"
#include <fstream>

WriteFile::WriteFile(const fs::path& filePath, const std::string& content)
    : filePath_(filePath)
    , content_(content)
{
}

TestActionResult WriteFile::process() const
{
    auto fileStream = std::ofstream(filePath_.string());
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try{
        fileStream.write(content_.c_str(), static_cast<std::streamsize>(content_.size()));
    } catch(std::exception& e)
    {
        return TestActionResult::Failure("File " + filePath_.string() + " writing error:" + e.what());
    }
    return TestActionResult::Success();
}
