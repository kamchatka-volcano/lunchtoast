#pragma once
#include <istream>
#include <string>
#include <utility>
#include <vector>

struct Section{
    std::string name;
    std::string value;
    bool isLast = false;
    bool isRaw = false;
    bool isVoid = false;
};

struct RestorableSection : public Section{
    RestorableSection() = default;
    RestorableSection(std::string name, std::string value, std::string pOriginalText, bool pIsComment = false)
        : Section{std::move(name), std::move(value)}
        , originalText(std::move(pOriginalText))
        , isComment(pIsComment)
    {
    }
    std::string originalText;
    bool isComment = false;
};

std::vector<Section> readSections(std::istream& input, const std::vector<std::string>& rawSectionsList = {});
std::vector<RestorableSection> readRestorableSections(std::istream& input, const std::vector<std::string>& rawSectionsList = {});
std::string readSectionValue(std::istream& input, const std::string& sectionName, bool isRaw = false);
