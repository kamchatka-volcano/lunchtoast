#include "filenamereader.h"
#include "utils.h"
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <iomanip>
#include <set>
#include <algorithm>

namespace {
std::vector<fs::path> getMatchingPaths(const fs::path& directory, const std::regex& pathFilter,
                                       std::function<bool(const fs::path& path)> pathMatchPredicate = [](const fs::path&){return true;});
}

FilenameGroup::FilenameGroup(const std::string& filenameOrRegexp, const fs::path& directory)
    : filenameOrRegexp_(filenameOrRegexp)
    , directory_(directory)
    , isRegexp_(false)
{
    if (boost::starts_with(filenameOrRegexp, "{") && boost::ends_with(filenameOrRegexp, "}")){
        fileMatchingRegexp_ = std::regex{filenameOrRegexp.substr(1, filenameOrRegexp.size() - 2)};
        isRegexp_ = true;
    }
}

std::vector<fs::path> FilenameGroup::fileList() const
{
    if (isRegexp_)
        return getMatchingPaths(directory_, fileMatchingRegexp_,
                                [](const fs::path& path) {return fs::is_regular_file(path);});
    else
        return {fs::absolute(directory_) / fs::path{filenameOrRegexp_}};
}

std::vector<fs::path> FilenameGroup::pathList() const
{
    auto result = std::vector<fs::path>{};
    if (isRegexp_)
        result = getMatchingPaths(directory_, fileMatchingRegexp_);
    else
        result = {fs::absolute(directory_) / fs::path{filenameOrRegexp_}};

    auto dirs = std::set<fs::path>{};
    for (const auto& path : result){
        auto parentDir = path.parent_path();
        while(!parentDir.empty() && parentDir != directory_){
            dirs.insert(parentDir);
            parentDir = parentDir.parent_path();
        }
    }
    std::copy(dirs.begin(), dirs.end(), std::back_inserter(result));
    return result;
}


std::string FilenameGroup::string() const
{
    return filenameOrRegexp_;
}

std::vector<FilenameGroup> readFilenames(const std::string& input, const fs::path& directory)
{
    auto result = std::vector<FilenameGroup>{};
    auto fileName = std::string{};
    auto stream = std::istringstream{input};
    while (stream >> std::quoted(fileName))
        result.push_back(FilenameGroup{fileName, directory});
    return result;
}

namespace{
std::vector<fs::path> getMatchingPaths(const fs::path& directory, const std::regex& pathFilter, std::function<bool(const fs::path&)> pathMatchPredicate)
{
    auto result = std::vector<fs::path>{};
    const auto paths = getDirectoryContent(directory);
    for(const auto& path : paths){
        if (!pathMatchPredicate(path))
            continue;
        auto match = std::smatch{};
        auto fileEntry = fs::relative(path, directory).string();
        if (std::regex_match(fileEntry, match, pathFilter))
            result.push_back(fs::absolute(directory) / fileEntry);
    }
    return result;
}
}
