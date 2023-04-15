#pragma once
#include <optional>
#include <string>
#include <variant>

namespace lunchtoast {

struct ProcessResultCheckMode {
    struct ExitCode {
        std::optional<int> value;
    };
    struct Output {
        std::string value;
    };
    struct ErrorOutput {
        std::string value;
    };

    template<typename T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, ProcessResultCheckMode>>* = nullptr>
    explicit ProcessResultCheckMode(T&& mode)
        : value{std::forward<T>(mode)}
    {
    }

    bool operator<(const ProcessResultCheckMode& rhs) const
    {
        return value.index() < rhs.value.index();
    }

    std::variant<ExitCode, Output, ErrorOutput> value;
};

} //namespace lunchtoast