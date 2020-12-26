#include "testresult.h"

TestResult TestResult::Success()
{
    auto result = TestResult{};
    result.type_ = TestResultType::Success;
    return result;
}

TestResult TestResult::Failure(const std::vector<std::string>& failedActionsMessages)
{
    auto result = TestResult{};
    result.type_ = TestResultType::Failure;
    result.failedActionsMessages_ = failedActionsMessages;
    return result;
}

TestResult TestResult::RuntimeError(const std::string& errorMsg, const std::vector<std::string>& failedActionsMessages)
{
    auto result = TestResult{};
    result.errorInfo_ = errorMsg;
    result.type_ = TestResultType::RuntimeError;
    result.failedActionsMessages_ = failedActionsMessages;
    return result;
}

TestResultType TestResult::type() const
{
    return type_;
}

const std::string& TestResult::errorInfo() const
{
    return errorInfo_;
}

const std::vector<std::string>& TestResult::failedActionsMessages() const
{
    return failedActionsMessages_;
}
