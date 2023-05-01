#include "comparefiles.h"
#include "utils.h"
#include <fmt/format.h>
#include <sfun/path.h>
#include <string>
#include <utility>

namespace lunchtoast {
namespace fs = std::filesystem;

enum class ComparisonResult {
    FilesEqual,
    FilesDontExist,
    FilesNotEqual
};

CompareFiles::CompareFiles(fs::path lhs, fs::path rhs, ComparisonMode mode)
    : lhs_{std::move(lhs)}
    , rhs_{std::move(rhs)}
    , mode_{mode}
{
}

namespace {

ComparisonResult compareFiles(const fs::path& lhs, const fs::path& rhs, ComparisonMode comparisonMode)
{
    if (!fs::exists(lhs) || !fs::exists(rhs))
        return ComparisonResult::FilesDontExist;

    const auto getFileContent = [&](const fs::path& path)
    {
        if (comparisonMode == ComparisonMode::Text)
            return readTextFile(path);
        return readFile(path);
    };

    if (getFileContent(lhs) != getFileContent(rhs))
        return ComparisonResult::FilesNotEqual;

    return ComparisonResult::FilesEqual;
}

std::string getFailedComparisonInfo(const ComparisonResult& result, const fs::path& lhs, const fs::path& rhs)
{
    if (result == ComparisonResult::FilesDontExist) {
        if (!fs::exists(lhs))
            return fmt::format("File {} doesn't exist", sfun::path_string(lhs.filename()));
        if (!fs::exists(rhs))
            return fmt::format("File {} doesn't exist", sfun::path_string(rhs.filename()));
        return {};
    }
    else if (result == ComparisonResult::FilesNotEqual)
        return fmt::format(
                "Files {} and {} aren't equal",
                sfun::path_string(lhs.filename()),
                sfun::path_string(rhs.filename()));
    return {};
}

} //namespace

TestActionResult CompareFiles::operator()() const
{
    const auto result = compareFiles(lhs_, rhs_, mode_);
    if (result != ComparisonResult::FilesEqual)
        return TestActionResult::Failure(getFailedComparisonInfo(result, lhs_, rhs_));
    return TestActionResult::Success();
}

} //namespace lunchtoast
