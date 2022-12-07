#pragma once
#include "testactionresult.h"
#include <sfun/interface.h>

namespace lunchtoast{

enum class TestActionType{
    RequiredOperation,
    Expectation,
    Assertion
};

class ITestAction : private sfun::Interface<ITestAction>{
public:
    virtual TestActionResult process() = 0;
    virtual TestActionType type() const = 0;
};

}