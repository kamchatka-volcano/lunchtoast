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
            if (!line.starts_with(separator))
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
        else if (!result.value.has_value() && ch == ':')
            result.value.emplace();
        else if (!result.value.has_value())
            result.name += ch;
        else
            result.value.value() += ch;
    }
    if (!result.value.has_value())
        result.name = sfun::trim_back(result.name);
    return result;
}

Section readSection(LineStream& stream, std::string_view multilineSectionSeparator)
{
    const auto lineNumber = stream.lineNumber();
    const auto line = stream.readLine();
    const auto [name, value] = readSectionLine(line);
    if (sfun::trim(name).empty())
        throw TestConfigError{lineNumber, "A section name can't be empty"};
    if (sfun::trim(name) != name)
        throw TestConfigError{lineNumber, "A section name can't start or end with whitespace characters"};

    if (value.has_value()) {
        if (sfun::trim(value.value()).empty()) {
            const auto multilineValue = readMultilineSectionValue(stream, multilineSectionSeparator);
            return {.name = name, //
                    .value = multilineValue.value,
                    .originalText = line + multilineValue.originalText};
        }
        return {.name = name, //
                .value = std::string{sfun::trim(value.value())},
                .originalText = line};
    }
    return {.name = name, //
            .value = {},
            .originalText = line};
}

std::string_view getMultilineSectionSeparator(const std::vector<Section>& sections)
{
    auto sectionIt = std::ranges::find_if(
            sections,
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
    auto sectionOuterSpace = std::string{};
    auto stream = LineStream{input};
    while (!stream.atEnd()) {
        auto line = stream.peekLine();
        if (line.starts_with("-")) {
            try {
                auto section = readSection(stream, getMultilineSectionSeparator(result));
                if (result.empty())
                    section.originalText.insert(0, sectionOuterSpace);
                else
                    result.back().originalText += sectionOuterSpace;
                sectionOuterSpace.clear();
                result.emplace_back(std::move(section));
            }
            catch (const TestConfigError& error) {
                readingError = error;
                return result;
            }
        }
        else {
            sectionOuterSpace += line;
            stream.skipLine();
        }
    }
    if (!result.empty())
        result.back().originalText += sectionOuterSpace;
    return result;
}

} //namespace lunchtoast