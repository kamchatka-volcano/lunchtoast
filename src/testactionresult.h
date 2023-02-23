#pragma once
#include <string>

namespace lunchtoast {

class TestActionResult {
public:
    static TestActionResult Success();
    static TestActionResult Failure(const std::string& errorInfo);

    bool isSuccessful() const;
    const std::string& errorInfo() const;

private:
    TestActionResult() = default;

private:
    bool isSuccessful_ = false;
    std::string errorInfo_;
};

} //namespace lunchtoast