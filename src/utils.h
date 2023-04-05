#pragma once
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
std::string processVariablesSubstitution(std::string value, const std::unordered_map<std::string, std::string>& vars);
std::vector<std::filesystem::path> getDirectoryContent(const std::filesystem::path& dir);
std::string homePathString(const std::filesystem::path& path);
std::string toLower(std::string_view str);
std::vector<std::string> splitCommand(const std::string& str);

class StringStream {
public:
    explicit StringStream(const std::string& str);
    void skip();
    [[nodiscard]] std::optional<char> read();
    [[nodiscard]] std::optional<char> peek();
    [[nodiscard]] bool atEnd();
    [[nodiscard]] std::string readUntil(std::function<bool(char ch)> pred);
    [[nodiscard]] std::string readUntil(std::function<bool(char ch)> pred, bool& stoppedAtEnd);

private:
    std::stringstream stream_;
};

} //namespace lunchtoast
