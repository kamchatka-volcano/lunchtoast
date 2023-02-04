#pragma once
#include <string>

namespace lunchtoast {

struct LaunchProcessResult {
    int exitCode;
    std::string output;
    std::string errorOutput;
};

} //namespace lunchtoast
