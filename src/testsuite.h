#pragma once
#include "alias_filesystem.h"
#include <vector>

struct TestSuite{
    std::vector<fs::path> tests;
    int passedTestsCounter = 0;
    int disabledTestsCounter = 0;
};
