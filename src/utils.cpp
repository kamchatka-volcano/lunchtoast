#include "utils.h"
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>
#include <stdlib.h>
#include <fstream>

using boost::uuids::detail::md5;

namespace {
std::string md5DigestToString(const md5::digest_type &digest)
{
    const auto charDigest = reinterpret_cast<const char *>(&digest);
    std::string result;
    boost::algorithm::hex(charDigest, charDigest + sizeof(md5::digest_type), std::back_inserter(result));
    return result;
}
}

std::string readFile(const fs::path& filePath)
{
    auto fileStream = std::ifstream{};
    fileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fileStream.open(filePath.string());
    auto buffer = std::stringstream{};
    buffer << fileStream.rdbuf();
    return buffer.str();
}

std::string calcMd5(const fs::path& filePath)
{
    return calcMd5(readFile(filePath));
}

std::string calcMd5(const std::string& data)
{
    auto hash = md5{};
    md5::digest_type digest;
    hash.process_bytes(data.data(), data.size());
    hash.get_digest(digest);
    return md5DigestToString(digest);
}

void processVariablesSubstitution(std::string& value,
                                  const std::string& varFileName,
                                  const std::string& varDirName)
{
    value = boost::replace_all_copy(value, "$filename$", varFileName);
    value = boost::replace_all_copy(value, "$dir$", varDirName);
}

std::vector<fs::path> getDirectoryContent(const fs::path& dir)
{
    auto result = std::vector<fs::path>{};
    auto end = fs::directory_iterator{};
    for (auto it = fs::directory_iterator{dir}; it != end; ++it){
        result.push_back(it->path());
        if (fs::is_directory(it->status())){
            auto subdirResult = getDirectoryContent(it->path());
            std::copy(subdirResult.begin(), subdirResult.end(), std::back_inserter(result));
        }
    }
    return result;
}

fs::path homePath(const fs::path& path)
{
    auto homePath = fs::path{getenv("HOME")};
    if (homePath.empty())
        return path;
    else
        return fs::relative(path, homePath);
}

std::string homePathString(const fs::path& path)
{
    auto resPath = homePath(path);
    if (resPath == path)
        return resPath.string();
    else if (resPath.string() == ".")
        return "~/";
    else
        return "~/" + resPath.string();
}