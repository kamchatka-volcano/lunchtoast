#pragma once
#include "launchprocessresult.h"
#include "testactionresult.h"
#include <gsl/pointers>
#include <optional>

namespace lunchtoast {

class CompareErrorOutput {
public:
    CompareErrorOutput(std::optional<LaunchProcessResult>& processResult, std::string expectedContent);
    TestActionResult operator()();

private:
    gsl::not_null<std::optional<LaunchProcessResult>*> processResult_;
    std::string expectedContent_;
};
} //namespace lunchtoast