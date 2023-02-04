#pragma once
#include "launchprocessresult.h"
#include "testactionresult.h"
#include <gsl/pointers>
#include <optional>

namespace lunchtoast {

class CompareExitCode {
public:
    CompareExitCode(std::optional<LaunchProcessResult>& processResult, int expectedExitCode);
    TestActionResult operator()();

private:
    gsl::not_null<std::optional<LaunchProcessResult>*> processResult_;
    int expectedExitCode_;
};
} //namespace lunchtoast