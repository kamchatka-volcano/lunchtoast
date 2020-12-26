#pragma once
#include "testactionresult.h"
#include "writefile.h"
#include "comparefiles.h"
#include "comparefilecontent.h"
#include "launchprocess.h"
#include <boost/variant.hpp>

enum class TestActionType{
    RequiredOperation,
    Expectation,
    Assertion
};

class TestAction
{
public:
    TestAction(const WriteFile& action);
    TestAction(const LaunchProcess& action);
    TestAction(const CompareFiles& action, TestActionType type);
    TestAction(const CompareFileContent& action, TestActionType type);
    TestActionResult process() const;
    TestActionType type() const;

private:
    boost::variant<WriteFile,
                   LaunchProcess,
                   CompareFiles,
                   CompareFileContent> action_;
    TestActionType type_;
};

