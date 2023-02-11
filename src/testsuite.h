#pragma once
#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace lunchtoast {

struct TestSuite {
    struct TestCfg {
        std::filesystem::path path;
        bool isEnabled;
    };
    std::vector<TestCfg> tests;
    int passedTestsCounter = 0;
    int disabledTestsCounter = 0;
};

}