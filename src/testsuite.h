#pragma once
#include "alias_filesystem.h"
#include <vector>

struct TestSuite{
    struct TestCfg{
        fs::path path;
        bool isEnabled;
    };
    std::vector<TestCfg> tests;
    int passedTestsCounter = 0;
    int disabledTestsCounter = 0;
};
