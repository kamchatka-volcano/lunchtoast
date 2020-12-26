#include "filenamesreader.h"
#include <sstream>
#include <iomanip>

std::vector<std::string> readFileNames(const std::string& input)
{
    auto fileNames = std::vector<std::string>{};
    auto fileName = std::string{};
    auto stream = std::istringstream{input};
    while (stream >> std::quoted(fileName))
        fileNames.push_back(fileName);
    return fileNames;
}
