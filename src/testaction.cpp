#include "testaction.h"

class ActionProcessor : public boost::static_visitor<TestActionResult>
{
public:
    TestActionResult operator()(const WriteFile& action) const
    {
        return action.process();
    }

    TestActionResult operator()(const LaunchProcess& action) const
    {
        return action.process();
    }

    TestActionResult operator()(const CompareFiles& action) const
    {
        return action.process();
    }

    TestActionResult operator()(const CompareFileContent& action) const
    {
        return action.process();
    }
};

TestAction::TestAction(const WriteFile& action)
    : action_(action)
    , type_(TestActionType::RequiredOperation)
{
}

TestAction::TestAction(const LaunchProcess& action)
    : action_(action)
    , type_(TestActionType::RequiredOperation)
{
}

TestAction::TestAction(const CompareFiles& action, TestActionType type)
    : action_(action)
    , type_(type)
{
}

TestAction::TestAction(const CompareFileContent& action, TestActionType type)
    : action_(action)
    , type_(type)
{
}

TestActionResult TestAction::process() const
{
    return boost::apply_visitor(ActionProcessor{}, action_);
}

TestActionType TestAction::type() const
{
    return type_;
}
