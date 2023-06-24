#include "wait.h"
#include "errors.h"
#include "utils.h"
#include <thread>

namespace lunchtoast {

Wait::Wait(std::chrono::milliseconds timePeriod)
    : timePeriod_{timePeriod}
{
}

TestActionResult Wait::operator()() const
{
    std::this_thread::sleep_for(timePeriod_);
    return TestActionResult::Success();
}

Wait makeWaitAction(const Section& section)
{
    auto time = readTime(section.value);
    if (!time.has_value())
        throw TestConfigError{"Wait section value must specify time duration (e.g. '500 ms')"};

    return Wait{time.value()};
}

}//namespace lunchtoast