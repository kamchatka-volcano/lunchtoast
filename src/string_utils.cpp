#include "string_utils.h"
#include <boost/algorithm/string.hpp>

namespace str{

std::vector<std::string> splitted(const std::string& str, const std::string& delim)
{
    auto result = std::vector<std::string>{};
    if (!delim.empty())
        boost::split(result, str, boost::is_any_of(delim));
    else
        boost::split(result, str, [](char ch){ return std::isspace(ch);});
    return result;
}


std::string before(const std::string &input, const std::string& marker)
{
    auto res = input.find(marker);
    if (res == std::string::npos)
        return input;
    return std::string(input.begin(), input.begin() + static_cast<int>(res));
}

std::string after(const std::string &input, const std::string& marker)
{
    auto res = input.find(marker);
    if (res == std::string::npos)
        return {};
    return std::string(input.begin() + static_cast<int>(res + marker.size()), input.end());
}


}
