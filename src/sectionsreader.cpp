#include "sectionsreader.h"
#include "errors.h"
#include "linestream.h"
#include <fmt/format.h>
#include <sfun/string_utils.h>
#include <gsl/util>
#include <algorithm>
#include <sstream>

namespace lunchtoast {

namespace {

struct MultilineValueReadResult {
    std::string originalText;
    std::string value;
};

MultilineValueReadResult readMultilineSectionValue(LineStream& stream, std::string_view separator)
{
    auto result = MultilineValueReadResult{};
    auto sectionStartLineNumber = stream.lineNumber() - 1;
    while (!stream.atEnd()) {
        auto lineNumber = stream.lineNumber();
        auto line = stream.readLine();
        result.originalText += line;
        if (sfun::trim(line) == separator) {
            if (!sfun::startsWith(line, separator))
                throw TestConfigError{
                        lineNumber,
                        "A multiline section separator must be placed at the start of a line"};
            if (!result.value.empty())
                result.value.pop_back();
            return result;
        }
        result.value += line;
    }
    throw TestConfigError{
            sectionStartLineNumber,
            fmt::format("A multiline section must be closed with '{}' separator", separator)};
}

std::string readUntil(std::stringstream& stream, char lastChar)
{
    auto ch = char{};
    auto result = std::string{};
    while (stream.get(ch)) {
        result += ch;
        if (ch == lastChar)
            return result;
    }
    return result;
}

struct LineReadResult {
    std::string name;
    std::optional<std::string> value;
};

LineReadResult readSectionLine(const std::string& line)
{
    auto result = LineReadResult{};
    auto lineStream = std::stringstream{line};

    auto ch = char{};
    lineStream.get(ch);
    Expects(ch == '-');

    while (lineStream.get(ch)) {
        if (!result.value.has_value() && (ch == '\"' || ch == '\'' || ch == '`')) {
            result.name += ch;
            result.name += readUntil(lineStream, ch);
        }
        else if (ch == ':')
            result.value.emplace();
        else if (!result.value.has_value())
            result.name += ch;
        else
            result.value.value() += ch;
    }
    return result;
}

Section readSection(LineStream& stream, std::string_view multilineSectionSeparator)
{
    auto result = Section{};
    auto lineNumber = stream.lineNumber();
    auto line = stream.readLine();
    result.originalText = line;

    auto [name, value] = readSectionLine(line);
    result.name = name;
    if (sfun::trim(result.name).empty())
        throw TestConfigError{lineNumber, "A section name can't be empty"};
    if (sfun::trim(result.name) != result.name)
        throw TestConfigError{lineNumber, "A section name can't start or end with whitespace characters"};

    if (value.has_value()) {
        result.value = sfun::trim(value.value());
        if (result.value.empty()) {
            auto multilineValue = readMultilineSectionValue(stream, multilineSectionSeparator);
            result.originalText += multilineValue.originalText;
            result.value = multilineValue.value;
        }
    }
    return result;
}

std::string_view getMultilineSectionSeparator(const std::vector<Section>& sections)
{
    auto sectionIt = std::find_if(
            sections.begin(),
            sections.end(),
            [](const auto& section)
            {
                return section.name == "Section separator";
            });
    if (sectionIt == sections.end())
        return "---";
    return sectionIt->value;
}

} //namespace
std::vector<Section> readSections(std::istream& input)
{
    auto error = SectionReadingError{};
    auto result = readSections(input, error);
    if (error)
        throw error.value();
    return result;
}

std::vector<Section> readSections(std::istream& input, SectionReadingError& readingError)
{
    auto result = std::vector<Section>{};
    auto sectionOuterWhitespace = std::string{};
    auto stream = LineStream{input};
    while (!stream.atEnd()) {
        auto line = stream.peekLine();
        if (sfun::startsWith(line, "-")) {
            try {
                auto section = readSection(stream, getMultilineSectionSeparator(result));
                if (result.empty())
                    section.originalText.insert(0, sectionOuterWhitespace);
                else
                    result.back().originalText += sectionOuterWhitespace;
                sectionOuterWhitespace.clear();
                result.emplace_back(std::move(section));
            }
            catch (const TestConfigError& error) {
                readingError = error;
                return result;
            }
        }
        else if (sfun::startsWith(line, "#") || sfun::trim(line).empty()) {
            sectionOuterWhitespace += line;
            stream.skipLine();
        }
        else {
            readingError = TestConfigError{
                    stream.lineNumber(),
                    "Space outside of sections can only contain whitespace characters and comments"};
            return result;
        }
    }
    if (!result.empty())
        result.back().originalText += sectionOuterWhitespace;
    return result;
}

} //namespace lunchtoast