#pragma once
#include "testactionresult.h"

enum class TestActionType{
    RequiredOperation,
    Expectation,
    Assertion
};

class TestAction
{
public:
    virtual ~TestAction() = default;
    virtual TestActionResult process() = 0;
    virtual TestActionType type() const = 0;
};

