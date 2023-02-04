#include "compareexitcode.h"
#include "errors.h"
#include "utils.h"
#include <fmt/format.h>
#include <utility>

namespace lunchtoast {

CompareExitCode::CompareExitCode(std::optional<LaunchProcessResult>& processResult, int expectedExitCode)
    : processResult_{&processResult}
    , expectedExitCode_{expectedExitCode}
{
}

TestActionResult CompareExitCode::operator()()
{
    if (!processResult_->has_value())
        throw TestConfigError{"No process was launched to check its result"};

    auto result = (processResult_->value().exitCode == expectedExitCode_);
    if (!result)
        return TestActionResult::Failure(fmt::format(
                "Process exit code '{}' isn't equal to the expected '{}'",
                processResult_->value().exitCode,
                expectedExitCode_));
    return TestActionResult::Success();
}

} //namespace lunchtoast
