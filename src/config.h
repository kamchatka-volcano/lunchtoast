#pragma once
#include <figcone/figcone.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace lunchtoast {

struct ExitCodeValue {
    std::optional<int> value;
};

} //namespace lunchtoast

namespace figcone {
template<>
struct StringConverter<lunchtoast::ExitCodeValue> {
    static std::optional<lunchtoast::ExitCodeValue> fromString(const std::string& val)
    {
        if (val == "*" || val == "any")
            return lunchtoast::ExitCodeValue{};
        return lunchtoast::ExitCodeValue{std::stoi(val)};
    }
};
} //namespace figcone

namespace lunchtoast {

using StringMap = std::unordered_map<std::string, std::string>;

struct TaggedVarsPreset : public figcone::Config {
    FIGCONE_PARAM(tag, std::string);
    FIGCONE_DICT(vars, StringMap);
};

struct Action : public figcone::Config {
    FIGCONE_PARAM(format, std::string);
    FIGCONE_PARAM(command, std::string);
    FIGCONE_PARAM(checkExitCode, ExitCodeValue)(ExitCodeValue{0});
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