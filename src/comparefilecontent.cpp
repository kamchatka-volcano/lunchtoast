#include "comparefilecontent.h"
#include "constants.h"
#include "utils.h"
#include <fmt/format.h>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <fstream>
#include <utility>

namespace lunchtoast {
namespace fs = std::filesystem;

CompareFileContent::CompareFileContent(
        fs::path filePath,
        std::string expectedFileContent,
        fs::path workingDir,
        int actionIndex)
    : filePath_{std::move(filePath)}
    , expectedFileContent_{std::move(expectedFileContent)}
    , workingDir_{std::move(workingDir)}
    , actionIndex_{actionIndex}
{
    expectedFileContent_ = sfun::replace(expectedFileContent_, "\r\n", "\n");
    expectedFileContent_ = sfun::replace(expectedFileContent_, "\r", "\n");
}

namespace {
std::string failureReportFilename(int actionIndex)
{
    return fmt::format(hardcoded::compareFileContentFailureReportFilename, actionIndex);
}

std::string generateComparisonFailureReport(
        std::string_view fileName,
        std::string_view fileContent,
        std::string_view expectedFileContent)
{
    return fmt::format(
            "-File: {}\n-Content:\n{}---\n-Expected content:\n{}---\n",
            fileName,
            fileContent.empty() ? "" : std::string{fileContent} + "\n",
            expectedFileContent.empty() ? "" : std::string{expectedFileContent} + "\n");
}

} //namespace

TestActionResult CompareFileContent::operator()() const
{
    if (!fs::exists(filePath_))
        return TestActionResult::Failure(fmt::format(
                "File content check has failed: {} doesn't exist",
                sfun::path_string(filePath_.filename())));

    const auto fileContent = readTextFile(filePath_);
    if (fileContent != expectedFileContent_) {
        if (expectedFileContent_.find('\n') != std::string::npos) {
            auto failureReportFile = std::ofstream{workingDir_ / failureReportFilename(actionIndex_)};
            failureReportFile << generateComparisonFailureReport(
                    sfun::path_string(filePath_),
                    fileContent,
                    expectedFileContent_);
            return TestActionResult::Failure(fmt::format(
                    "File {} content isn't equal to the expected string. More info in {}",
                    sfun::path_string(filePath_.filename()),
                    failureReportFilename(actionIndex_)));
        }
        else
            return TestActionResult::Failure(fmt::format(
                    "File {} content isn't equal to the expected string '{}'",
                    sfun::path_string(filePath_.filename()),
                    expectedFileContent_));
    }

    return TestActionResult::Success();
}

} //namespace lunchtoast
