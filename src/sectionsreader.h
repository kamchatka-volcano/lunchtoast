#pragma once
#include "section.h"
#include "errors.h"
#include <string>
#include <vector>
#include <istream>
#include <optional>

namespace lunchtoast{

class SectionReadingError{
public:
    operator bool()
    {
        return error_.has_value();
    }
    TestConfigError value()
    {
        return *error_;
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

}