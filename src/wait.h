#pragma once
#include "testactionresult.h"
#include "section.h"
#include <filesystem>
#include <string>
#include <chrono>

namespace lunchtoast {

class Wait {
public:
    explicit Wait(std::chrono::milliseconds timePeriod);
    TestActionResult operator()() const;

private:
    std::chrono::milliseconds timePeriod_;
};

Wait makeWaitAction(const Section&);

} //namespace lunchtoast
