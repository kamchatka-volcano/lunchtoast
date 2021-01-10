#pragma once
#include <istream>
#include <string>
#include <vector>

struct Section{
    std::string name;
    std::string value;
    bool isVoid = false;
};

struct RawSectionSpecifier{
    std::string name;
    std::string end;
};

struct RestorableSection : public Section{
    RestorableSection()
    {
    }
    RestorableSection(const std::string& pName, const std::string& pValue, const std::string& pOriginalText, bool pIsComment = false)
        : originalText(pOriginalText)
        , isComment(pIsComment)
    {
        name = pName;
        value = pValue;
    }
    std::string originalText;
    bool isComment = false;
};

std::vector<Section> readSections(std::istream& input, const std::vector<RawSectionSpecifier>& rawSectionsList = {});
std::vector<RestorableSection> readRestorableSections(std::istream& input, const std::vector<RawSectionSpecifier>& rawSectionsList = {});
std::string readSectionValue(std::istream& input, const std::string& sectionName, bool isRaw = false);
