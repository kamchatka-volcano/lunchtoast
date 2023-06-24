#pragma once
#include <string_view>

namespace lunchtoast::hardcoded {
using namespace std::string_view_literals;

inline constexpr auto appVersion = "lunchtoast v0.4.0"sv;
inline constexpr auto testCaseFilename = "test.toast"sv;
inline constexpr auto configFilename = "lunchtoast.cfg"sv;
inline constexpr auto launchFailureReportFilename = "launch_{}.failure_info"sv;
inline constexpr auto compareFileContentFailureReportFilename = "compare_file_content_{}.failure_info"sv;

} //namespace lunchtoast::hardcoded