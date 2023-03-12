#include "utils.h"
#include <platform_folders.h>
#include <fmt/format.h>
#include <sfun/contract.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <sfun/utility.h>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>


namespace lunchtoast {
namespace fs = std::filesystem;

std::string readTextFile(const fs::path& filePath)
{
    auto fileStream = std::ifstream{filePath, std::ios::binary};
    if (!fileStream.is_open())
        throw std::runtime_error{fmt::format("Can't open {}", sfun::pathString(filePath))};
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    auto buffer = std::stringstream{};
    buffer << fileStream.rdbuf();
    auto result = sfun::replace(buffer.str(), "\r\n", "\n");
    return sfun::replace(result, "\r", "\n");
}

std::string readFile(const fs::path& filePath)
{
    auto fileStream = std::ifstream{filePath, std::ios::binary};
    if (!fileStream.is_open())
        throw std::runtime_error{fmt::format("Can't open {}", sfun::pathString(filePath))};

    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    auto buffer = std::stringstream{};
    buffer << fileStream.rdbuf();
    return buffer.str();
}

std::string processVariablesSubstitution(std::string value, const std::unordered_map<std::string, std::string>& vars)
{
    for (const auto& [varName, varValue] : vars)
        value = std::regex_replace(value, std::regex{R"(\$\{\{\s*)" + varName + R"(\s*\}\})"}, varValue);
    return value;
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
    return sfun::makePath(sago::getDesktopFolder()).parent_path();
}
} //namespace

std::string homePathString(const fs::path& path)
{
    auto resPath = fs::relative(path, homePath());
    if (resPath == path)
        return sfun::pathString(resPath);
    else if (sfun::pathString(resPath) == ".")
        return "~/";
    else
        return "~/" + sfun::pathString(resPath);
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
    auto pos = sfun::index_t{};
    auto partPos = std::optional<sfun::index_t>{};

    auto insideString = false;
    auto isQuotationAtStart = true;
    auto addCommandPart = [&]
    {
        sfunPrecondition(partPos.has_value());
        result.emplace_back(
                std::next(str.data(), *partPos),
                static_cast<std::size_t>(pos - *partPos + (isQuotationAtStart ? 0 : 1)));
        partPos = std::nullopt;
    };
    for (; pos < sfun::ssize(str); ++pos) {
        if (!insideString && sfun::isspace(str.at(pos))) {
            if (partPos.has_value())
                addCommandPart();
            continue;
        }
        if (str.at(pos) == '"') {
            if (insideString)
                addCommandPart();
            else {
                isQuotationAtStart = !partPos.has_value();
                if (pos != sfun::ssize(str) - 1 && sfun::isspace(str.at(pos + 1)))
                    partPos = pos + 1;
            }

            insideString = !insideString;
            continue;
        }
        if (!sfun::isspace(str.at(pos)) && !partPos.has_value())
            partPos = pos;
    }
    if (insideString)
        return {};

    if (partPos.has_value())
        addCommandPart();

    return result;
}

} //namespace lunchtoast