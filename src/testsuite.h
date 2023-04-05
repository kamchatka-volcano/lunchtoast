#pragma once
#include "useraction.h"
#include <filesystem>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace lunchtoast {

struct TestCfg {
    std::filesystem::path path;
    bool isEnabled;
    std::unordered_map<std::string, std::string> vars;
    std::vector<UserAction> userActions;
};

struct TestSuite {
    std::vector<TestCfg> tests;
    int passedTestsCounter = 0;
    int disabledTestsCounter = 0;
};

} //namespace lunchtoast