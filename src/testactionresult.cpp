#include "testactionresult.h"

TestActionResult TestActionResult::Success()
{
    auto result = TestActionResult{};
    result.isSuccessful_ = true;
    return result;
}

TestActionResult TestActionResult::Failure(const std::string& errorInfo)
{
    auto result = TestActionResult{};
    result.isSuccessful_ = false;
    result.errorInfo_ = errorInfo;
    return result;
}

bool TestActionResult::isSuccessful() const
{
    return isSuccessful_;
}

const std::string& TestActionResult::errorInfo() const
{
    return errorInfo_;
}
