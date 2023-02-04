#include "compareerroroutput.h"
#include "errors.h"
#include <fmt/format.h>
#include <utility>

namespace lunchtoast {

CompareErrorOutput::CompareErrorOutput(std::optional<LaunchProcessResult>& processResult, std::string expectedContent)
    : processResult_{&processResult}
    , expectedContent_(std::move(expectedContent))
{
}

TestActionResult CompareErrorOutput::operator()()
{
    if (!processResult_->has_value())
        throw TestConfigError{"No process was launched to check its result"};

    auto result = (processResult_->value().errorOutput == expectedContent_);
    if (!result)
        return TestActionResult::Failure(fmt::format(
                "Process error output '{}' isn't equal to the expected '{}'",
                processResult_->value().errorOutput,
                expectedContent_));
    return TestActionResult::Success();
}

} //namespace lunchtoast
