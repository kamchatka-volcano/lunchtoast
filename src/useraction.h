#pragma once
#include "config.h"
#include "processresultcheckmode.h"
#include "testactiontype.h"
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <vector>

namespace lunchtoast {

class UserAction {
public:
    UserAction(const Action& action);
    std::optional<std::string> makeCommand(
            const std::string& action,
            const std::unordered_map<std::string, std::string>& vars,
            const std::string& inputParam) const;
    std::set<ProcessResultCheckMode> makeProcessResultCheckModeSet(
            const std::unordered_map<std::string, std::string>& vars,
            const std::string& inputParam) const;
    TestActionType actionType() const;

private:
    std::regex format_;
    std::string commandFormat_;
    std::vector<int> paramsOrder_;
    std::set<ProcessResultCheckMode> processResultCheckModeSet_;
    TestActionType actionType_;
};

} //namespace lunchtoast
