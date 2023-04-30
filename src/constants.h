#pragma once
#include <string_view>

namespace lunchtoast::hardcoded {

inline constexpr std::string_view testCaseFilename = "test.toast";
inline constexpr std::string_view configFilename = "lunchtoast.cfg";
inline constexpr std::string_view launchFailureReportFilename = "launch_{}.failure_info";
inline constexpr std::string_view compareFileContentFailureReportFilename = "compare_file_content_{}.failure_info";

} //namespace lunchtoast::hardcoded