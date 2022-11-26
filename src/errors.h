#pragma once
#include <fmt/format.h>
#include <stdexcept>

namespace lunchtoast{

struct Error : public std::runtime_error
{
    Error(int lineNumber, std::string_view errorMessage)
        : std::runtime_error{fmt::format("line {}: {}", lineNumber, errorMessage)}
    {}
};

}