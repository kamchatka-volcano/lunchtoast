#pragma once
#include <algorithm>
#include <filesystem>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace lunchtoast {

std::string readTextFile(const std::filesystem::path& filePath);
std::string readFile(const std::filesystem::path& filePath);
std::vector<std::filesystem::path> readFilenames(const std::string& input, const std::filesystem::path& directory);
std::string processVariablesSubstitution(std::string value, const std::unordered_map<std::string, std::string>& vars);
std::vector<std::filesystem::path> getDirectoryContent(const std::filesystem::path& dir);
std::string homePathString(const std::filesystem::path& path);
std::string toLower(std::string_view str);
std::vector<std::string> splitCommand(const std::string& str);
std::vector<std::string> splitSectionValue(const std::string& str);
std::unordered_map<std::string, std::string> readInputParamSections(const std::string&);
std::string normalizeLineEndings(std::string_view str);

template<typename TContainer>
    requires requires(TContainer t) {
                 {
                     std::begin(t)
                 } -> std::same_as<typename TContainer::iterator>;
                 {
                     std::end(t)
                 } -> std::same_as<typename TContainer::iterator>;
             }
bool contains(const TContainer& range, const TContainer& subrange)
{
    return std::search(std::begin(range), std::end(range), std::begin(subrange), std::end(subrange)) != std::end(range);
}

class StringStream {
public:
    explicit StringStream(const std::string& str);
    void skip();
    std::optional<char> read();
    std::optional<char> peek();
    bool atEnd();
    std::string readUntil(std::function<bool(char ch)> pred);
    std::string readUntil(std::function<bool(char ch)> pred, bool& stoppedAtEnd);

private:
    std::stringstream stream_;
};

} //namespace lunchtoast
