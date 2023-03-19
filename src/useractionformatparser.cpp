#include "useractionformatparser.h"
#include "errors.h"
#include <fmt/format.h>
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

class StringStream {
public:
    explicit StringStream(const std::string& str)
        : stream_{str}
    {
    }

    template<int size = 1>
    auto read()
    {
        static_assert(size > 0);

        auto ch = char{};
        if constexpr (size == 1) {
            if (!stream_.get(ch))
                return std::optional<char>{};
            else
                return std::optional<char>{ch};
        }
        else {
            auto result = std::string{};
            for (auto i = 0; i < size; ++i) {
                if (!stream_.get(ch))
                    return std::optional<std::string>{};
                result.push_back(ch);
            }
            return std::optional<std::string>{result};
        }
    }

    template<int size = 1>
    auto peek()
    {
        static_assert(size > 0);
        auto ch = char{};
        auto pos = stream_.tellg();
        auto restorePosition = gsl::finally(
                [&]
                {
                    stream_.seekg(pos);
                });

        if constexpr (size == 1) {
            if (!stream_.get(ch)) {
                stream_.clear();
                return std::optional<char>{};
            }
            return std::optional<char>{ch};
        }
        else {
            auto result = std::string{};
            for (auto i = 0; i < size; ++i) {
                if (!stream_.get(ch)) {
                    stream_.clear();
                    return std::optional<std::string>{};
                }
                result.push_back(ch);
            }
            return std::optional<std::string>{result};
        }
    }

    template<int size = 1>
    void skip()
    {
        static_assert(size > 0);
        read<size>();
    }

    bool atEnd()
    {
        return !peek().has_value();
    }

    std::string readUntil(std::function<bool(char ch)> pred)
    {
        auto result = std::string();
        while (!atEnd()) {
            auto ch = peek().value();
            if (!pred(ch))
                break;
            result.push_back(ch);
            skip();
        }
        return result;
    }

private:
    std::stringstream stream_;
};

namespace {

std::vector<Token> parseUserActionFormat(const std::string& format)
{
    auto result = std::vector<Token>{};
    auto stream = StringStream{format};
    while (!stream.atEnd()) {
        auto ch = stream.read().value();
        if (ch == '%') {
            auto param = stream.readUntil(sfun::isdigit);
            if (!param.empty()) {
                if (!result.empty() && std::holds_alternative<StringToken>(result.back())) {
                    auto& str = std::get<StringToken>(result.back()).value;
                    if (!str.empty() && sfun::isspace(str.back())) {
                        str = sfun::trimBack(str);
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
    auto paramsSet = std::set(format.paramsOrder.begin(), format.paramsOrder.end());
    if (paramsSet.size() != format.paramsOrder.size())
        throw formatParamsError();
    if (*paramsSet.begin() != 1)
        throw formatParamsError();
    if (*std::prev(paramsSet.end()) != sfun::ssize(paramsSet))
        throw formatParamsError();
}

} //namespace

UserActionFormat makeUserActionFormat(const std::string& format)
{
    auto tokens = parseUserActionFormat(format);
    auto result = UserActionFormat{};
    auto tokenVisitor = sfun::overloaded{
            [&](const WhitespaceToken&)
            {
                result.formatRegex += R"(\s+)";
            },
            [&](const ParamToken& token)
            {
                if (std::find(result.paramsOrder.begin(), result.paramsOrder.end(), token.index) !=
                    result.paramsOrder.end())
                    throw ActionFormatError{
                            format,
                            fmt::format(
                                    "encountered multiple parameters %{}, format parameters indices must be unique",
                                    token.index)};
                result.paramsOrder.push_back(token.index);
                result.formatRegex += R"((.+))";
            },
            [&](const StringToken& token)
            {
                result.formatRegex += token.value;
            }};

    for (const auto& token : tokens)
        std::visit(tokenVisitor, token);

    validateUserActionFormat(format, result);
    return result;
}

} //namespace lunchtoast