#include "useractionformatparser.h"
#include "errors.h"
#include "utils.h"
#include <fmt/format.h>
#include <range/v3/range/conversion.hpp>
#include <sfun/contract.h>
#include <sfun/functional.h>
#include <sfun/string_utils.h>
#include <gsl/util>
#include <functional>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <variant>

namespace lunchtoast {

struct StringToken {
    std::string value;
};

struct WhitespaceToken {};

struct ParamToken {
    int index;
};

using Token = std::variant<StringToken, WhitespaceToken, ParamToken>;

namespace {

std::vector<Token> parseUserActionFormat(const std::string& format)
{
    auto result = std::vector<Token>{};
    auto stream = StringStream{format};
    while (!stream.atEnd()) {
        auto ch = stream.read().value();
        if (ch == '%') {
            auto param = stream.readUntil(std::not_fn(sfun::isdigit));
            if (!param.empty()) {
                if (!result.empty() && std::holds_alternative<StringToken>(result.back())) {
                    auto& str = std::get<StringToken>(result.back()).value;
                    if (!str.empty() && sfun::isspace(str.back())) {
                        str = sfun::trim_back(str);
                        result.emplace_back(WhitespaceToken{});
                    }
                }
                auto paramIndex = std::stoi(param);
                result.emplace_back(ParamToken{paramIndex});
                continue;
            }
        }
        if (sfun::isspace(ch)) {
            if (!result.empty() && std::holds_alternative<WhitespaceToken>(result.back()))
                continue;

            if (!result.empty() && std::holds_alternative<ParamToken>(result.back())) {
                result.emplace_back(WhitespaceToken{});
                continue;
            }
        }

        if (result.empty() || !std::holds_alternative<StringToken>(result.back()))
            result.emplace_back(StringToken{});
        std::get<StringToken>(result.back()).value.push_back(ch);
    }
    return result;
}

void validateUserActionFormat(std::string_view formatString, const UserActionFormat& format)
{
    if (format.paramsOrder.empty())
        return;
    const auto formatParamsError = [&]
    {
        return ActionFormatError{
                formatString,
                "format parameters indices must form a continuous range starting with 1"};
    };
    auto paramsSet = format.paramsOrder | ranges::to<std::set>;
    if (std::ssize(paramsSet) != std::ssize(format.paramsOrder))
        throw formatParamsError();
    if (*paramsSet.begin() != 1)
        throw formatParamsError();
    if (*std::prev(paramsSet.end()) != std::ssize(paramsSet))
        throw formatParamsError();
}

} //namespace

UserActionFormat makeUserActionFormat(const std::string& format)
{
    auto tokens = parseUserActionFormat(format);
    auto result = UserActionFormat{};
    auto lastToken = false;
    auto tokenVisitor = sfun::overloaded{
            [&](const WhitespaceToken&)
            {
                result.formatRegex += R"(\s+)";
            },
            [&](const ParamToken& token)
            {
                if (std::ranges::find(result.paramsOrder, token.index) != result.paramsOrder.end())
                    throw ActionFormatError{
                            format,
                            fmt::format(
                                    "encountered multiple parameters %{}, format parameters indices must be unique",
                                    token.index)};
                result.paramsOrder.push_back(token.index);
                result.formatRegex += lastToken ? R"((.+))" : R"((.+?))";
            },
            [&](const StringToken& token)
            {
                result.formatRegex += token.value;
            }};

    for (auto i = 0; i < std::ssize(tokens); ++i) {
        lastToken = (i == std::ssize(tokens) - 1);
        std::visit(tokenVisitor, tokens.at(i));
    }

    validateUserActionFormat(format, result);
    return result;
}

} //namespace lunchtoast