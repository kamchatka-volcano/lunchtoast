#pragma once
#include <sfun/member.h>
#include <functional>
#include <istream>
#include <string>

namespace lunchtoast {

class LineStream {
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
    sfun::member<std::istream&> stream_;
    int lineNumber_ = 1;
};

} //namespace lunchtoast
