#include "utils.h"
#include "errors.h"
#include <platform_folders.h>
#include <fmt/format.h>
#include <sfun/contract.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <sfun/utility.h>
#include <gsl/util>
#include <fstream>
#include <optional>
#include <regex>

namespace lunchtoast {
namespace fs = std::filesystem;

StringStream::StringStream(const std::string& str)
    : stream_{str}
{
}

std::optional<char> StringStream::read()
{
    auto ch = char{};
    if (!stream_.get(ch))
        return std::nullopt;
    return ch;
}

std::optional<char> StringStream::peek()
{
    auto ch = char{};
    auto pos = stream_.tellg();
    auto restorePosition = gsl::finally(
            [&]
            {
                stream_.seekg(pos);
            });

    if (!stream_.get(ch)) {
        stream_.clear();
        return std::nullopt;
    }
    return ch;
}

void StringStream::skip()
{
    [[maybe_unused]] auto res = read();
}

bool StringStream::atEnd()
{
    return !peek().has_value();
}

std::string StringStream::readUntil(std::function<bool(char ch)> pred)
{
    auto discard = false;
    return readUntil(pred, discard);
}

std::string StringStream::readUntil(std::function<bool(char ch)> pred, bool& stoppedAtEnd)
{
    auto result = std::string();
    while (!atEnd()) {
        auto ch = peek().value();
        if (pred(ch)) {
            stoppedAtEnd = false;
            return result;
        }
        result.push_back(ch);
        skip();
    }
    stoppedAtEnd = true;
    return result;
}

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

std::vector<std::string> splitCommand(const std::string& str)
{
    auto result = std::vector<std::string>{};
    auto stream = StringStream{str};
    auto prevCh = char{};
    while (!stream.atEnd()) {
        auto ch = stream.read().value();
        if (ch == '\"' || ch == '\'' || ch == '`') {
            auto stoppedAtEnd = false;
            auto quotedText = stream.readUntil(
                    [quotationMark = ch](char ch)
                    {
                        return ch == quotationMark;
                    },
                    stoppedAtEnd);
            if (stoppedAtEnd)
                throw TestConfigError{fmt::format("Command '{}' has an unclosed quotation mark", str)};

            if (!prevCh || sfun::isspace(prevCh))
                result.emplace_back(std::move(quotedText));
            else
                result.back() += quotedText;
            stream.skip();
        }
        else if (!sfun::isspace(ch)) {
            if (!prevCh || sfun::isspace(prevCh))
                result.emplace_back();
            result.back() += ch;
        }
        prevCh = ch;
    }
    return result;
}

} //namespace lunchtoast