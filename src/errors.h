#pragma once
#include <fmt/format.h>
#include <stdexcept>

namespace lunchtoast{

struct TestConfigError : public std::runtime_error
{
    explicit TestConfigError(std::string_view errorMessage)
        : std::runtime_error{std::string{errorMessage}}
    {}

    TestConfigError(int lineNumber, std::string_view errorMessage)
        : std::runtime_error{fmt::format("line {}: {}", lineNumber, errorMessage)}
    {}
};

}