#pragma once
#include <figcone/config.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace lunchtoast {

using StringMap = std::unordered_map<std::string, std::string>;

struct TaggedVarsPreset : public figcone::Config {
    FIGCONE_PARAM(tag, std::string);
    FIGCONE_DICT(vars, StringMap);
};

struct Config : public figcone::Config {
    FIGCONE_DICT(vars, StringMap);
    FIGCONE_NODELIST(tagVars, std::vector<TaggedVarsPreset>)();
};

} //namespace lunchtoast