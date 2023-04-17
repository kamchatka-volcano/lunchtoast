#include "useraction.h"
#include "useractionformatparser.h"
#include "utils.h"
#include <range/v3/range/conversion.hpp>
#include <range/v3/view.hpp>
#include <sfun/functional.h>
#include <sfun/string_utils.h>
#include <sfun/utility.h>

namespace lunchtoast {
namespace views = ranges::views;

UserAction::UserAction(const Action& action)
    : commandFormat_{action.command}
    , actionType_{action.stopOnFailure ? TestActionType::Assertion : TestActionType::Expectation}
{
    auto [formatRegexStr, paramsOrder] = makeUserActionFormat(action.format);
    format_ = std::regex{formatRegexStr};
    paramsOrder_ = std::move(paramsOrder);
    processResultCheckModeSet_.emplace(ProcessResultCheckMode::ExitCode{action.checkExitCode});
    if (action.checkOutput.has_value())
        processResultCheckModeSet_.emplace(ProcessResultCheckMode::Output{action.checkOutput.value()});
    if (action.checkErrorOutput.has_value())
        processResultCheckModeSet_.emplace(ProcessResultCheckMode::ErrorOutput{action.checkErrorOutput.value()});
}

std::optional<std::string> UserAction::makeCommand(
        const std::string& action,
        const std::unordered_map<std::string, std::string>& vars,
        const std::string& inputParam) const
{
    auto match = std::smatch{};
    if (!std::regex_match(action, match, format_))
        return std::nullopt;

    auto command = commandFormat_;
    for (auto i = 0; i < std::ssize(match) - 1; ++i)
        command = sfun::replace(command, "%" + std::to_string(paramsOrder_.at(i)), match[i + 1].str());

    command = sfun::replace(command, "%input", inputParam);
    command = processVariablesSubstitution(command, vars);
    return command;
}

std::set<ProcessResultCheckMode> UserAction::makeProcessResultCheckModeSet(
        const std::unordered_map<std::string, std::string>& vars,
        const std::string& inputParam) const
{
    const auto checkModeVisitor = sfun::overloaded{
            [](ProcessResultCheckMode::ExitCode&) {},
            [&](auto& checkMode)
            {
                checkMode.value = sfun::replace(checkMode.value, "%input", inputParam);
                checkMode.value = processVariablesSubstitution(checkMode.value, vars);
            }};
    const auto processCheckMode = [&](ProcessResultCheckMode checkMode)
    {
        std::visit(checkModeVisitor, checkMode.value);
        return checkMode;
    };
    return processResultCheckModeSet_ | views::transform(processCheckMode) | ranges::to<std::set>;
}

TestActionType UserAction::actionType() const
{
    return actionType_;
}

} //namespace lunchtoast