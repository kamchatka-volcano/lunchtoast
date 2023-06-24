#pragma once
#include "comparefilecontent.h"
#include "comparefiles.h"
#include "launchprocess.h"
#include "testactiontype.h"
#include "wait.h"
#include "writefile.h"
#include <variant>

namespace lunchtoast {

class TestAction {
public:
    template<typename TAction>
    TestAction(TAction action, TestActionType actionType)
        : action_{std::move(action)}
        , actionType_{actionType}
    {
    }

    template<typename TFuncOnSuccess, typename TFuncOnFailure, typename TFuncOnRuntimeError>
    void process(TFuncOnSuccess onSuccess, TFuncOnFailure onFailure, TFuncOnRuntimeError onRuntimeError) const
    {
        auto processAction = [&](auto& action)
        {
            try {
                auto result = action();
                if (result.isSuccessful())
                    onSuccess(action);
                else
                    onFailure(action, result.errorInfo());
            }
            catch (const std::exception& e) {
                onRuntimeError(action, e.what());
            }
        };
        std::visit(processAction, action_);
    }

    template<typename TAction>
    bool is() const
    {
        return std::holds_alternative<TAction>(action_);
    }

    TestActionType type() const
    {
        return actionType_;
    }

private:
    std::variant<CompareFileContent, CompareFiles, LaunchProcess, WriteFile, Wait> action_;
    TestActionType actionType_;
};

} //namespace lunchtoast