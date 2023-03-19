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

struct Action : public figcone::Config {
    FIGCONE_PARAM(format, std::string);
    FIGCONE_PARAM(command, std::string);
    FIGCONE_PARAM(checkExitCode, int)(0);
    FIGCONE_PARAM(checkOutput, figcone::optional<std::string>);
    FIGCONE_PARAM(checkErrorOutput, figcone::optional<std::string>);
    FIGCONE_PARAM(stopOnFailure, bool)(true);
};

struct Config : public figcone::Config {
    FIGCONE_DICT(vars, StringMap)();
    FIGCONE_NODELIST(tagVars, std::vector<TaggedVarsPreset>)();
    FIGCONE_NODELIST(actions, std::vector<Action>)();
};

} //namespace lunchtoast