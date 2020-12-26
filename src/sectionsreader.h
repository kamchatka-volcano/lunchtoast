#pragma once
#include <istream>
#include <string>
#include <vector>

struct Section{
    std::string name;
    std::string value;
};

std::vector<Section> readSections(std::istream& input, const std::vector<std::string>& rawSectionsList = {});
std::string readSectionValue(std::istream& input, const std::string& sectionName, bool isRaw = false);
