#include "linestream.h"
#include <string>
#include <istream>
#include <functional>

namespace lunchtoast{

LineStream::LineStream(std::istream& stream)
    : stream_{stream}
{}

void LineStream::skipLine()
{
    readLine();
}

std::string LineStream::readLine()
{
    auto ch = char{};
    auto line = std::string{};
    while (stream().get(ch)){
        line += ch;
        if (ch == '\n') {
            lineNumber_++;
            return line;
        }
    }
    stream().clear();
    lineNumber_++;
    return line;
}

std::string LineStream::peekLine()
{
    auto pos = stream().tellg();
    auto line = readLine();
    stream().seekg(pos);
    lineNumber_--;
    return line;
}

bool LineStream::atEnd()
{
    auto pos = stream().tellg();
    auto ch = char{};
    stream().get(ch);
    auto result = stream().eof();
    stream().seekg(pos);
    return result;
}

int LineStream::lineNumber() const
{
    return lineNumber_;
}

std::istream& LineStream::stream()
{
    return stream_;
}

}

