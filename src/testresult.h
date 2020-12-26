#pragma once
#include <string>
#include <vector>

enum class TestResultType{
    Success,
    Failure,
    RuntimeError
};

class TestResult{
public:
    static TestResult Success();
    static TestResult Failure(const std::vector<std::string>& failedActionsMessages);
    static TestResult RuntimeError(const std::string& errorMsg, const std::vector<std::string>& failedActionsMessages);

    TestResultType type() const;
    const std::string& errorInfo() const;
    const std::vector<std::string>& failedActionsMessages() const;

private:
    TestResult() = default;

private:
    TestResultType type_;
    std::string errorInfo_;
    std::vector<std::string> failedActionsMessages_;
};
