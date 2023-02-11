#include "filenamereader.h"
#include "utils.h"
#include <sfun/string_utils.h>
#include <sstream>
#include <iomanip>
#include <set>
#include <algorithm>
#include <utility>
#include <functional>

namespace lunchtoast {
namespace fs = std::filesystem;

namespace {
std::vector<fs::path> getMatchingPaths(const fs::path& directory, const std::regex& pathFilter,
                                       const std::function<bool(const fs::path& path)>& pathMatchPredicate = [](
                                               const fs::path&) { return true; });
}

FilenameGroup::FilenameGroup(std::string filenameOrRegexp, fs::path directory)
        : filenameOrRegexp_(std::move(filenameOrRegexp)), directory_(std::move(directory)), isRegexp_(false)
{
    if (sfun::startsWith(filenameOrRegexp_, "{") && sfun::endsWith(filenameOrRegexp_, "}")) {
        fileMatchingRegexp_ = std::regex{filenameOrRegexp_.substr(1, filenameOrRegexp_.size() - 2)};
        isRegexp_ = true;
    }
}

std::vector<fs::path> FilenameGroup::fileList() const
{
    if (isRegexp_)
        return getMatchingPaths(directory_, fileMatchingRegexp_,
                                [](const fs::path& path) { return fs::is_regular_file(path); });
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
    for (const auto& path: result) {
        auto parentDir = path.parent_path();
        while (!parentDir.empty() && parentDir != directory_) {
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
        result.emplace_back(fileName, directory);
    return result;
}

namespace {
std::vector<fs::path> getMatchingPaths(const fs::path& directory, const std::regex& pathFilter,
                                       const std::function<bool(const fs::path&)>& pathMatchPredicate)
{
    auto result = std::vector<fs::path>{};
    const auto paths = getDirectoryContent(directory);
    for (const auto& path: paths) {
        if (!pathMatchPredicate(path))
            continue;
        auto match = std::smatch{};
        auto fileEntry = fs::relative(path, directory).string();
        auto unixFileEntry = sfun::replace(fileEntry, "/", "\\");
        auto windowsFileEntry = sfun::replace(fileEntry, "\\", "/");
        if (std::regex_match(unixFileEntry, match, pathFilter) || std::regex_match(windowsFileEntry, match, pathFilter))
            result.push_back(fs::absolute(directory) / fileEntry);
    }
    return result;
}
}

}