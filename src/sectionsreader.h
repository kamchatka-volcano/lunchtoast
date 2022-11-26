#pragma once
#include "section.h"
#include <string>
#include <vector>
#include <istream>

namespace lunchtoast{

std::vector<Section> readSections(std::istream& input);

}