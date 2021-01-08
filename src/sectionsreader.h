#pragma once
#include <istream>
#include <string>
#include <vector>

struct Section{
    std::string name;
    std::string value;
};

struct RawSection : public Section{
    RawSection()
    {
    }
    RawSection(const std::string& pName, const std::string& pValue, const std::string& pOriginalText, bool pIsComment = false)
        : originalText(pOriginalText)
        , isComment(pIsComment)
    {
        name = pName;
        value = pValue;
    }
    std::string originalText;
    bool isComment = false;
};

std::vector<Section> readSections(std::istream& input, const std::vector<std::string>& rawSectionsList = {});
std::vector<RawSection> readRawSections(std::istream& input, const std::vector<std::string>& rawSectionsList = {});
std::string readSectionValue(std::istream& input, const std::string& sectionName, bool isRaw = false);
