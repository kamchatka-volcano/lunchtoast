#include "comparefiles.h"
#include "utils.h"
#include "alias_boost_filesystem.h"
#include <spdlog/fmt/fmt.h>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <string>
#include <sstream>

namespace{
std::string filenameListStr(const std::vector<fs::path>& pathList);
}

CompareFiles::CompareFiles(const FilenameGroup& lhs,
                           const FilenameGroup& rhs)
    : lhs_(lhs)
    , rhs_(rhs)    
{
}

TestActionResult CompareFiles::process() const
{
    auto lhsPaths = lhs_.fileList();
    std::sort(lhsPaths.begin(), lhsPaths.end());
    auto rhsPaths = rhs_.fileList();
    std::sort(rhsPaths.begin(), rhsPaths.end());
    if (lhsPaths.size() != rhsPaths.size()){
        return TestActionResult::Failure(
                fmt::format("Files equality check has failed, file lists have different number of elements:\n{} : {}\n{} : {}\n",
                lhs_.string(), filenameListStr(lhsPaths), rhs_.string(), filenameListStr(rhsPaths)));
    }
    auto result = true;
    auto errorInfo = std::vector<std::string>{};
    for (auto i = 0u; i < lhsPaths.size(); ++i){
        auto failedComparisonInfo = std::string{};
        if(!compareFiles(lhsPaths[i], rhsPaths[i], failedComparisonInfo)){
            errorInfo.push_back(failedComparisonInfo);
            result = false;
        }
    }
    if (result)
        return TestActionResult::Success();
    else
        return TestActionResult::Failure(boost::join(errorInfo, "\n"));
}

bool CompareFiles::compareFiles(const fs::path& lhs, const fs::path& rhs, std::string& failedComparisonInfo) const
{
    auto lhsExists = fs::exists(lhs);
    auto rhsExists = fs::exists(rhs);
    auto bothFilesExist = lhsExists && rhsExists;
    if (!bothFilesExist)
        failedComparisonInfo += fmt::format("Files {} and {} equality check has failed, ", lhs_.string(), rhs_.string());
    if (!lhsExists)
        failedComparisonInfo += fmt::format("file {} doesn't exist; ", lhs.filename().string());
    if (!rhsExists)
        failedComparisonInfo += fmt::format("file {} doesn't exist; ", rhs.filename().string());
    if (!bothFilesExist)
        return false;

    if (calcMd5(lhs) != calcMd5(rhs)){
        failedComparisonInfo += fmt::format("Files {} and {} equality check has failed, files {} and {} are different",
                                            lhs_.string(), rhs_.string(), lhs.filename().string(), rhs.filename().string());
        return false;
    }
    return true;
}

namespace{

std::string filenameListStr(const std::vector<fs::path>& pathList)
{
    auto filenameList = std::vector<std::string>{};
    std::transform(pathList.begin(), pathList.end(), std::back_inserter(filenameList),
                   [](const fs::path& path){return path.filename().string();});
    return boost::join(filenameList, ",");
}

}


