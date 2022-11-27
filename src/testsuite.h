#pragma once
#include <filesystem>
#include <vector>


namespace lunchtoast{

struct TestSuite{
    struct TestCfg{
        std::filesystem::path path;
        bool isEnabled;
    };
    std::vector<TestCfg> tests;
    int passedTestsCounter = 0;
    int disabledTestsCounter = 0;
};

}