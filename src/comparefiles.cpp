#include "comparefiles.h"
#include "utils.h"
#include <fmt/format.h>
#include <sfun/string_utils.h>
#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

namespace lunchtoast {
namespace fs = std::filesystem;

CompareFiles::CompareFiles(FilenameGroup lhs, FilenameGroup rhs)
    : lhs_(std::move(lhs))
    , rhs_(std::move(rhs))
{
}

namespace {
std::string filenameListStr(const std::vector<fs::path>& pathList)
{
    auto pathToString = [](const fs::path& path)
    {
        return toString(path.filename());
    };
    auto filenameList = std::vector<std::string>{};
    std::transform(pathList.begin(), pathList.end(), std::back_inserter(filenameList), pathToString);
    return sfun::join(filenameList, ",");
}
} //namespace

TestActionResult CompareFiles::operator()()
{
    auto lhsPaths = lhs_.fileList();
    std::sort(lhsPaths.begin(), lhsPaths.end());
    auto rhsPaths = rhs_.fileList();
    std::sort(rhsPaths.begin(), rhsPaths.end());
    if (lhsPaths.size() != rhsPaths.size()) {
        return TestActionResult::Failure(fmt::format(
                "Files equality check has failed, file lists have different number of elements:\n{} : {}\n{} : {}\n",
                lhs_.string(),
                filenameListStr(lhsPaths),
                rhs_.string(),
                filenameListStr(rhsPaths)));
    }
    auto result = true;
    auto errorInfo = std::vector<std::string>{};
    for (auto i = 0u; i < lhsPaths.size(); ++i) {
        auto failedComparisonInfo = std::string{};
        if (!compareFiles(lhsPaths[i], rhsPaths[i], failedComparisonInfo)) {
            errorInfo.push_back(failedComparisonInfo);
            result = false;
        }
    }
    if (result)
        return TestActionResult::Success();
    else
        return TestActionResult::Failure(sfun::join(errorInfo, "\n"));
}

bool CompareFiles::compareFiles(const fs::path& lhs, const fs::path& rhs, std::string& failedComparisonInfo) const
{
    auto lhsExists = fs::exists(lhs);
    auto rhsExists = fs::exists(rhs);
    auto bothFilesExist = lhsExists && rhsExists;
    if (!bothFilesExist)
        failedComparisonInfo +=
                fmt::format("Files {} and {} equality check has failed, ", lhs_.string(), rhs_.string());
    if (!lhsExists)
        failedComparisonInfo += fmt::format("file {} doesn't exist; ", toString(lhs.filename()));
    if (!rhsExists)
        failedComparisonInfo += fmt::format("file {} doesn't exist; ", toString(rhs.filename()));
    if (!bothFilesExist)
        return false;

    if (readFile(lhs) != readFile(rhs)) {
        failedComparisonInfo += fmt::format(
                "Files {} and {} equality check has failed, files {} and {} are different",
                lhs_.string(),
                rhs_.string(),
                toString(lhs.filename()),
                toString(rhs.filename()));
        return false;
    }
    return true;
}

} //namespace lunchtoast
