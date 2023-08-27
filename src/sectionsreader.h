#pragma once
#include "errors.h"
#include "section.h"
#include <istream>
#include <optional>
#include <string>
#include <vector>

namespace lunchtoast {

class SectionReadingError {
public:
    operator bool()
    {
        return error_.has_value();
    }

    TestConfigError value()
    {
        return error_.value();
    }

    void operator=(const TestConfigError& error)
    {
        error_ = error;
    }

private:
    std::optional<TestConfigError> error_;
};

std::vector<Section> readSections(std::istream& input);
std::vector<Section> readSections(std::istream& input, SectionReadingError&);

} //namespace lunchtoast