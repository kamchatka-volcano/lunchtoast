#pragma once
#include <string>
#include <vector>

namespace str{
std::vector<std::string> splitted(const std::string& str, const std::string& delim = {});
std::string before(const std::string &input, const std::string& marker);
std::string after(const std::string &input, const std::string& marker);
}
