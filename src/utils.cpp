#include "utils.h"
#include <sfun/string_utils.h>
#include <boost/process/env.hpp>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace lunchtoast {
namespace fs = std::filesystem;

std::string readFile(const fs::path& filePath)
{
    auto fileStream = std::ifstream{};
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fileStream.open(filePath.string());
    auto buffer = std::stringstream{};
    buffer << fileStream.rdbuf();
    return buffer.str();
}

void processVariablesSubstitution(std::string& value, const std::string& varFileName, const std::string& varDirName)
{
    value = sfun::replace(value, "$filename$", varFileName);
    value = sfun::replace(value, "$dir$", varDirName);
}

std::vector<fs::path> getDirectoryContent(const fs::path& dir)
{
    auto result = std::vector<fs::path>{};
    auto end = fs::directory_iterator{};
    for (auto it = fs::directory_iterator{dir}; it != end; ++it) {
        result.push_back(it->path());
        if (fs::is_directory(it->status())) {
            auto subdirResult = getDirectoryContent(it->path());
            std::copy(subdirResult.begin(), subdirResult.end(), std::back_inserter(result));
        }
    }
    return result;
}

fs::path homePath(const fs::path& path)
{

    //auto homePath = fs::path{getenv("HOME")};
    auto homePath = boost::process::environment()["HOME"];
    if (homePath.empty())
        return path;
    else
        return fs::relative(path, homePath.to_string());
}

std::string homePathString(const fs::path& path)
{
    auto resPath = homePath(path);
    if (resPath == path)
        return resPath.string();
    else if (resPath.string() == ".")
        return "~/";
    else
        return "~/" + resPath.string();
}

std::string toLower(std::string_view str)
{
    auto result = std::string{};
    std::transform(
            str.begin(),
            str.end(),
            std::back_inserter(result),
            [](char ch)
            {
                return sfun::tolower(ch);
            });
    return result;
}

std::vector<std::string_view> splitCommand(std::string_view str)
{
    if (str.empty())
        return std::vector<std::string_view>{str};

    auto result = std::vector<std::string_view>{};
    auto pos = std::size_t{0};
    auto partPos = std::string_view::npos;
    auto addCommandPart = [&]()
    {
        result.emplace_back(std::string_view{std::next(str.data(), partPos), pos - partPos});
        partPos = std::string_view::npos;
    };

    auto insideString = false;
    for (; pos < str.size(); ++pos) {
        if (!insideString && sfun::isspace(str.at(pos))) {
            if (partPos != std::string_view::npos)
                addCommandPart();
            continue;
        }
        if (str.at(pos) == '"') {
            if (insideString)
                addCommandPart();
            insideString = !insideString;
            continue;
        }
        if (!sfun::isspace(str.at(pos)) && partPos == std::string_view::npos)
            partPos = pos;
    }
    if (insideString)
        return {};

    if (partPos != std::string_view::npos)
        addCommandPart();

    return result;
}

} //namespace lunchtoast