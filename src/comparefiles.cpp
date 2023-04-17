#include "comparefiles.h"
#include "utils.h"
#include <fmt/format.h>
#include <range/v3/action.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <sfun/path.h>
#include <gsl/util>
#include <algorithm>
#include <string>
#include <utility>
#include <variant>

namespace lunchtoast {
namespace fs = std::filesystem;
namespace views = ranges::views;

CompareFiles::CompareFiles(FilenameGroup lhs, FilenameGroup rhs, ComparisonMode mode)
    : lhs_{std::move(lhs)}
    , rhs_{std::move(rhs)}
    , mode_{mode}
{
}

namespace {

std::string filenameListStr(const std::vector<fs::path>& pathList)
{
    const auto pathToString = [](const fs::path& path)
    {
        return sfun::pathString(path.filename());
    };
    return pathList | views::transform(pathToString) | views::join(',') | ranges::to<std::string>;
}
enum class ComparisonResult {
    FilesEqual,
    FilesDontExist,
    FilesNotEqual
};

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

std::string getFailedComparisonInfo(const std::tuple<ComparisonResult, fs::path, fs::path>& result)
{
    const auto& [res, lhs, rhs] = result;
    if (res == ComparisonResult::FilesDontExist) {
        if (!fs::exists(lhs))
            return fmt::format("File {} doesn't exist", sfun::pathString(lhs.filename()));
        if (!fs::exists(rhs))
            return fmt::format("File {} doesn't exist", sfun::pathString(rhs.filename()));
        return {};
    }
    else if (res == ComparisonResult::FilesNotEqual)
        return fmt::format(
                "Files {} and {} aren't equal",
                sfun::pathString(lhs.filename()),
                sfun::pathString(rhs.filename()));
    return {};
}

} //namespace

TestActionResult CompareFiles::operator()()
{
    const auto lhsPaths = lhs_.fileList() | ranges::actions::sort;
    const auto rhsPaths = rhs_.fileList() | ranges::actions::sort;

    if (std::ssize(lhsPaths) != std::ssize(rhsPaths)) {
        return TestActionResult::Failure(fmt::format(
                "Files equality check has failed, file lists have different number of elements:\n{} : {}\n{} : {}\n",
                lhs_.string(),
                filenameListStr(lhsPaths),
                rhs_.string(),
                filenameListStr(rhsPaths)));
    }

    const auto makeComparison = [this](const auto& lhsRhs)
    {
        const auto& [lhsPath, rhsPath] = lhsRhs;
        return std::make_tuple(compareFiles(lhsPath, rhsPath, mode_), lhsPath, rhsPath);
    };

    const auto isNotEqual = [](const std::tuple<ComparisonResult, fs::path, fs::path>& result)
    {
        return std::get<0>(result) != ComparisonResult::FilesEqual;
    };

    const auto errorInfo = views::zip(lhsPaths, rhsPaths) | //
            views::transform(makeComparison) | //
            views::filter(isNotEqual) | //
            views::transform(getFailedComparisonInfo) | //
            views::join('\n') | ranges::to<std::string>;

    if (errorInfo.empty())
        return TestActionResult::Success();
    else
        return TestActionResult::Failure(errorInfo);
}

} //namespace lunchtoast
