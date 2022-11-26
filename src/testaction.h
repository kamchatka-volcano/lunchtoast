#pragma once
#include "testactionresult.h"


namespace lunchtoast{

enum class TestActionType{
    RequiredOperation,
    Expectation,
    Assertion
};

class TestAction{
public:
    virtual ~TestAction() = default;
    virtual TestActionResult process() = 0;
    virtual TestActionType type() const = 0;
};

}