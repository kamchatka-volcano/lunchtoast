#include "sectionsreader.h"
#include "string_utils.h"
#include <boost/algorithm/string.hpp>
#include <algorithm>

namespace {
    void readSectionFirstLine(Section& section, const std::string& line, int lineIndex)
    {
        auto delimiterPos = line.find(":");
        if (delimiterPos == std::string::npos)
            throw std::runtime_error("line#" + std::to_string(lineIndex + 1) +
                                     ": Section's first line must contain a name delimiter ':'");

        section.name = line.substr(1, delimiterPos - 1);
        if (section.name.empty())
            throw std::runtime_error("line#" + std::to_string(lineIndex + 1) +
                                     ": Section must have a name");

        if (delimiterPos < line.size())
            section.value = line.substr(delimiterPos + 1);
        if (std::all_of(section.value.begin(), section.value.end(), isspace))
            section.value.clear();
    }

    bool containsAnySubstr(const std::string& str, const std::vector<std::string>& subStrList)
    {
        return std::any_of(subStrList.begin(),
                           subStrList.end(),
                           [&str](const std::string& subStr){
                               return str.find(subStr) != std::string::npos;
                           });
    }
}


std::vector<Section> readSections(std::istream& input, const std::vector<std::string>& rawSectionsList)
{
    auto sections = std::vector<Section>{};
    auto line = std::string{};
    auto lineIndex = -1;
    while(std::getline(input, line)){
        lineIndex++;
        line+="\n";
        auto isSectionRaw = !sections.empty() &&
                             containsAnySubstr(sections.back().name, rawSectionsList);
        if (!isSectionRaw && line[0] == '#') //skip comment line
            continue;
        if (line[0] == '-'){
            sections.emplace_back();
            readSectionFirstLine(sections.back(), line, lineIndex);
            continue;
        }
        if (sections.empty())
            continue;
        auto& section = sections.back();
        section.value += line;
    }
    return sections;
}

std::vector<RawSection> readRawSections(std::istream& input, const std::vector<std::string>& rawSectionsList)
{
    auto sections = std::vector<RawSection>{};
    auto line = std::string{};
    auto lineIndex = -1;
    while(std::getline(input, line)){
        lineIndex++;
        line+="\n";
        auto isSectionRaw = !sections.empty() &&
                            !sections.back().isComment &&
                            containsAnySubstr(sections.back().name, rawSectionsList);
        if (!isSectionRaw && line[0] == '#'){
            if (sections.empty()){
                auto commentSection = RawSection{};
                commentSection.isComment = true;
                commentSection.originalText = line;
                sections.push_back(commentSection);
            }
            else
                sections.back().originalText += line;
            continue;
        }
        if (line[0] == '-'){
            sections.emplace_back();
            readSectionFirstLine(sections.back(), line, lineIndex);
            sections.back().originalText += line;
            continue;
        }
        if (sections.empty() || sections.back().isComment)
            continue;
        auto& section = sections.back();
        section.value += line;
        section.originalText += line;
    }
    return sections;
}

std::string readSectionValue(std::istream& input, const std::string& sectionName, bool isRaw)
{
    auto result = std::string{};
    auto line = std::string{};
    auto sectionIsFound = false;
    while(std::getline(input, line)){
        line+="\n";
        if (sectionIsFound){
            if (boost::starts_with(line, "-"))
                return result;
            else{
                if (!isRaw && line[0] == '#') //skip comment line
                    continue;
                result += line;
                continue;
            }
        }
        if (boost::starts_with(line, "-" + sectionName)){
            sectionIsFound = true;
            result += str::after(line, ":");
            if (std::all_of(result.begin(), result.end(), isspace))
                result.clear();
            continue;
        }
    }
    return result;
}
