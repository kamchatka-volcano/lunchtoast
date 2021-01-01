#include "comparefiles.h"
#include "utils.h"
#include "alias_boost_filesystem.h"
#include <boost/algorithm/string.hpp>
#include <regex>
#include <string>
#include <sstream>
#include <iostream>


namespace{
std::vector<fs::path> getMatchingPaths(const fs::path& directory, const std::regex& pathFilter);
std::string filenameListStr(const std::vector<fs::path>& pathList);
bool compareFiles(const fs::path& lhs, const fs::path& rhs, std::string& failedComparisonInfo);
}

ComparedFiles::ComparedFiles(const fs::path& filePath)
    : filePath_(filePath)
{
    const auto filename = filePath.filename().string();
    if (boost::starts_with(filename, "{") && boost::ends_with(filename, "}")){
        fileMatchingRegexp_ = std::regex{filename.substr(1, filename.size() - 2)};
        isRegexp_ = true;
    }
}

std::vector<fs::path> ComparedFiles::pathList() const
{
    if (isRegexp_)
        return getMatchingPaths(filePath_.parent_path(), fileMatchingRegexp_);
    else
        return {filePath_};
}

std::string ComparedFiles::string() const
{
    return filePath_.filename().string();
}

CompareFiles::CompareFiles(const fs::path& lhs,
                           const fs::path& rhs)
    : lhs_(lhs)
    , rhs_(rhs)    
{
}

TestActionResult CompareFiles::process() const
{
    auto lhsPaths = lhs_.pathList();
    std::sort(lhsPaths.begin(), lhsPaths.end());
    auto rhsPaths = rhs_.pathList();
    std::sort(rhsPaths.begin(), rhsPaths.end());
    if (lhsPaths.size() != rhsPaths.size()){
        return TestActionResult::Failure(
                "Files equality check has failed, file lists have different number of elements:\n" +
                lhs_.string() + " : " + filenameListStr(lhsPaths) + "\n" +
                rhs_.string() + " : " + filenameListStr(rhsPaths) + "\n");
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

namespace{
std::vector<fs::path> getMatchingPaths(const fs::path& directory, const std::regex& pathFilter)
{
    auto result = std::vector<fs::path>{};
    const auto end = fs::directory_iterator{};
    for(auto it = fs::directory_iterator{directory}; it != end; ++it){
        if(!fs::is_regular_file(it->status()))
            continue;
        auto match = std::smatch{};
        auto fileEntry = it->path().filename().string();
        if (std::regex_match(fileEntry, match, pathFilter))
            result.push_back(fs::absolute(fileEntry, directory));
    }
    return result;
}

std::string filenameListStr(const std::vector<fs::path>& pathList)
{
    auto filenameList = std::vector<std::string>{};
    std::transform(pathList.begin(), pathList.end(), std::back_inserter(filenameList),
                   [](const fs::path& path){return path.filename().string();});
    return boost::join(filenameList, ",");
}

bool compareFiles(const fs::path& lhs, const fs::path& rhs, std::string& failedComparisonInfo)
{
    auto lhsExists = fs::exists(lhs);
    auto rhsExists = fs::exists(rhs);
    auto bothFilesExist = lhsExists && rhsExists;
    if (!bothFilesExist)
        failedComparisonInfo += "Files equality check has failed, ";
    if (!lhsExists)
        failedComparisonInfo += "file " + lhs.filename().string() + " doesn't exist; ";
    if (!rhsExists)
        failedComparisonInfo += "file " + rhs.filename().string() + " doesn't exist; ";
    if (!bothFilesExist)
        return false;

    if (calcMd5(lhs) != calcMd5(rhs)){
        failedComparisonInfo += "Files equality check has failed, files "
                  + lhs.filename().string() + " and "
                  + rhs.filename().string() + " are different";
        return false;
    }
    return true;
}

}


