#pragma once
#include <string>

class TestActionResult{
public:
    static TestActionResult Success();
    static TestActionResult Failure(const std::string& errorInfo);

    bool isSuccessful() const;
    const std::string& errorInfo() const;

private:
    TestActionResult() = default;

private:
    bool isSuccessful_;
    std::string errorInfo_;
};
