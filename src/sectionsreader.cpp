#include "sectionsreader.h"
#include <sfun/string_utils.h>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <cassert>

namespace str = sfun::string_utils;

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

    bool startsWithAny(const std::string& str, const std::vector<std::string>& subStrList)
    {
        return std::any_of(subStrList.begin(),
                           subStrList.end(),
                           [&str](const std::string& subStr){
                               return boost::starts_with(str, subStr);
                           });
    }

    bool isRaw(const std::string& sectionName, const std::vector<std::string>& rawSectionsList)
    {
        return startsWithAny(sectionName, rawSectionsList);
    }
}


std::vector<Section> readSections(std::istream& input, const std::vector<std::string>& rawSectionsList)
{
    auto sections = std::vector<Section>{};
    auto line = std::string{};
    auto lineIndex = -1;
    auto isSectionRaw = false;
    while(std::getline(input, line)){
        lineIndex++;
        if (!input.eof())
            line+="\n";
        if (!sections.empty())
            isSectionRaw = isRaw(sections.back().name, rawSectionsList);

        if (!isSectionRaw && line[0] == '#') //skip comment line
            continue;

        if (isSectionRaw && boost::starts_with(line, "---")){
            sections.emplace_back();
            sections.back().isRaw = true;
            sections.back().isVoid = true;
            continue;
        }
        if (!isSectionRaw && line[0] == '-'){
            sections.emplace_back();
            readSectionFirstLine(sections.back(), line, lineIndex);
            continue;
        }
        if (sections.empty())
            continue;

        sections.back().value += line;
    }
    sections.erase(std::remove_if(sections.begin(), sections.end(), [](const Section& section){ return section.isVoid;}), sections.end());
    if (!sections.empty())
        sections.back().isLast = true;
    return sections;
}

std::vector<RestorableSection> readRestorableSections(std::istream& input, const std::vector<std::string>& rawSectionsList)
{
    auto sections = std::vector<RestorableSection>{};
    auto line = std::string{};
    auto lineIndex = -1;
    auto isSectionRaw = false;
    while(std::getline(input, line)){
        lineIndex++;
        line+="\n";
        if (!sections.empty())
            isSectionRaw = !sections.back().isComment && isRaw(sections.back().name, rawSectionsList);

        if (!isSectionRaw && line[0] == '#'){
            if (sections.empty()){
                auto commentSection = RestorableSection{};
                commentSection.isComment = true;
                commentSection.originalText = line;
                sections.push_back(commentSection);
            }
            else
                sections.back().originalText += line;
            continue;
        }
        if (isSectionRaw && boost::starts_with(line, "---")){
            sections.emplace_back();
            sections.back().originalText = line;
            sections.back().isComment = true;
            continue;
        }
        if (!isSectionRaw && line[0] == '-'){
            sections.emplace_back();
            readSectionFirstLine(sections.back(), line, lineIndex);
            sections.back().originalText = line;
            continue;
        }
        if (sections.empty())
            continue;
        auto& section = sections.back();
        section.originalText += line;
        if (!section.isComment)
            section.value += line;
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
