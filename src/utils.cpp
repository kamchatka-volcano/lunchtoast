#include "utils.h"
#include <platform_folders.h>
#include <sfun/string_utils.h>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace lunchtoast {
namespace fs = std::filesystem;

std::string readTextFile(const fs::path& filePath)
{
    auto fileStream = std::ifstream{filePath, std::ios::binary};
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    auto buffer = std::stringstream{};
    buffer << fileStream.rdbuf();
    auto result = sfun::replace(buffer.str(), "\r\n", "\n");
    return sfun::replace(result, "\r", "\n");
}

std::string readFile(const fs::path& filePath)
{
    auto fileStream = std::ifstream{filePath, std::ios::binary};
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
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

namespace {
std::filesystem::path homePath()
{
    return toPath(sago::getDesktopFolder()).parent_path();
}
} //namespace

std::string homePathString(const fs::path& path)
{
    auto resPath = fs::relative(path, homePath());
    if (resPath == path)
        return toString(resPath);
    else if (toString(resPath) == ".")
        return "~/";
    else
        return "~/" + toString(resPath);
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

    auto insideString = false;
    auto isQuotationAtStart = true;
    auto addCommandPart = [&]
    {
        result.emplace_back(
                std::string_view{std::next(str.data(), partPos), pos - partPos + (isQuotationAtStart ? 0 : 1)});
        partPos = std::string_view::npos;
    };
    for (; pos < str.size(); ++pos) {
        if (!insideString && sfun::isspace(str.at(pos))) {
            if (partPos != std::string_view::npos)
                addCommandPart();
            continue;
        }
        if (str.at(pos) == '"') {
            if (insideString)
                addCommandPart();
            else {
                isQuotationAtStart = (partPos == std::string_view::npos);
                if (pos != str.size() - 1 && sfun::isspace(str.at(pos + 1)))
                    partPos = pos + 1;
            }

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

std::filesystem::path toPath(std::string_view str)
{
#ifdef _WIN32
    return toUtf16(str);
#else
    return str;
#endif
}

std::string toString(const std::filesystem::path& path)
{
#ifdef _WIN32
    return toUtf8(path.wstring());
#else
    return path.string();
#endif
}

#ifdef _WIN32
std::string toUtf8(std::wstring_view utf16String)
{
    int count = WideCharToMultiByte(
            CP_UTF8,
            0,
            utf16String.data(),
            static_cast<int>(utf16String.size()),
            nullptr,
            0,
            nullptr,
            nullptr);
    auto utf8String = std::string(count, 0);
    WideCharToMultiByte(CP_UTF8, 0, utf16String.data(), -1, &utf8String[0], count, nullptr, nullptr);
    return utf8String;
}

std::wstring toUtf16(std::string_view utf8String)
{
    int count = MultiByteToWideChar(CP_UTF8, 0, utf8String.data(), static_cast<int>(utf8String.size()), nullptr, 0);
    auto utf16String = std::wstring(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8String.data(), static_cast<int>(utf8String.size()), &utf16String[0], count);
    return utf16String;
}
#endif

} //namespace lunchtoast