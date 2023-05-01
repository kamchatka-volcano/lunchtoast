#include "filenamegroup.h"
#include "utils.h"
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <sfun/path.h>
#include <sfun/string_utils.h>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <set>
#include <sstream>
#include <utility>

namespace lunchtoast {
namespace views = ranges::views;
namespace fs = std::filesystem;

namespace {
std::vector<fs::path> getMatchingPaths(
        const fs::path& directory,
        const std::regex& pathFilter,
        const std::function<bool(const fs::path& path)>& pathMatchPredicate =
                [](const fs::path&)
        {
            return true;
        });
}

FilenameGroup::FilenameGroup(std::string filenameOrRegexp, fs::path directory)
    : filenameOrRegexp_(std::move(filenameOrRegexp))
    , directory_(std::move(directory))
    , isRegexp_(false)
{
    if (filenameOrRegexp_.starts_with('{') && filenameOrRegexp_.ends_with('}')) {
        fileMatchingRegexp_ = std::regex{filenameOrRegexp_.substr(1, std::ssize(filenameOrRegexp_) - 2)};
        isRegexp_ = true;
    }
}

std::vector<fs::path> FilenameGroup::fileList() const
{
    if (isRegexp_)
        return getMatchingPaths(
                directory_,
                fileMatchingRegexp_,
                [](const fs::path& path)
                {
                    return fs::is_regular_file(path);
                });
    else
        return {fs::weakly_canonical(directory_ / sfun::make_path(filenameOrRegexp_))};
}

std::vector<fs::path> FilenameGroup::pathList() const
{
    auto result = std::vector<fs::path>{};
    if (isRegexp_)
        result = getMatchingPaths(directory_, fileMatchingRegexp_);
    else
        result = {fs::weakly_canonical(directory_ / sfun::make_path(filenameOrRegexp_))};

    auto dirs = std::set<fs::path>{};
    for (const auto& path : result) {
        auto parentDir = path.parent_path();
        while (!parentDir.empty() && parentDir != directory_) {
            dirs.insert(parentDir);
            parentDir = parentDir.parent_path();
        }
    }
    std::ranges::copy(dirs, std::back_inserter(result));
    return result;
}

std::string FilenameGroup::string() const
{
    return filenameOrRegexp_;
}

std::vector<FilenameGroup> readFilenameGroups(const std::string& input, const fs::path& directory)
{
    auto result = std::vector<FilenameGroup>{};
    auto fileName = std::string{};
    auto stream = std::istringstream{input};
    while (stream >> std::quoted(fileName))
        result.emplace_back(fileName, directory);
    return result;
}

namespace {
std::vector<fs::path> getMatchingPaths(
        const fs::path& directory,
        const std::regex& pathFilter,
        const std::function<bool(const fs::path&)>& pathMatchPredicate)
{
    auto result = std::vector<fs::path>{};
    const auto pathMatchesFilter = [&](const fs::path& path) -> fs::path
    {
        const auto fileEntry = sfun::path_string(fs::relative(path, directory));
        const auto unixFileEntry = sfun::replace(fileEntry, "/", "\\");
        const auto windowsFileEntry = sfun::replace(fileEntry, "\\", "/");
        auto match = std::smatch{};
        if (std::regex_match(unixFileEntry, match, pathFilter) || std::regex_match(windowsFileEntry, match, pathFilter))
            return fs::weakly_canonical(directory / sfun::make_path(fileEntry));
        return {};
    };
    const auto pathNotEmpty = [](const auto& path)
    {
        return !path.empty();
    };
    const auto directoryContent = getDirectoryContent(directory);
    return directoryContent | //
            views::filter(pathMatchPredicate) | //
            views::transform(pathMatchesFilter) | //
            views::filter(pathNotEmpty) | //
            ranges::to<std::vector>;
}

} //namespace

} //namespace lunchtoast