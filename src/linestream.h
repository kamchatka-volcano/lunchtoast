#pragma once
#include <string>
#include <istream>
#include <functional>

namespace lunchtoast{

class LineStream{
public:
    explicit LineStream(std::istream& stream);
    void skipLine();
    std::string readLine();
    std::string peekLine();
    bool atEnd();
    int lineNumber() const;

private:
    std::istream& stream();

private:
    std::reference_wrapper<std::istream> stream_;
    int lineNumber_ = 1;
};

}

