#include "sectionsreader.h"
#include "errors.h"
#include "linestream.h"
#include <sfun/string_utils.h>
#include <fmt/format.h>
#include <range/v3/algorithm.hpp>
#include <gsl/util>


namespace lunchtoast{

namespace str = sfun::string_utils;

namespace {

void readMultilineSectionValue(Section& section, LineStream& stream, const std::string& separator)
{
    auto sectionStartLineNumber = stream.lineNumber() - 1;
    while (!stream.atEnd()) {
        auto lineNumber = stream.lineNumber();
        auto line = stream.readLine();
        section.originalText += line;
        if (str::trim(line) == separator) {
            if (!str::startsWith(line, separator))
                throw TestConfigError{lineNumber, "A multiline section separator must be placed at the start of a line"};
            if (!section.value.empty())
                section.value.pop_back();
            return;
        }
        section.value += line;
    }
    throw TestConfigError{sectionStartLineNumber,
                fmt::format("A multiline section must be closed with '{}' separator", separator)};
}

Section readSection(LineStream& stream, const std::string& multilineSectionSeparator)
{
    auto result = Section{};
    auto lineNumber = stream.lineNumber();
    auto line = stream.readLine();
    Expects(str::startsWith(line, "-"));
    if (line.find(':') == std::string::npos)
        throw TestConfigError{lineNumber, "A section must contain ':' after its name"};

    result.originalText = line;
    result.name = str::before(str::after(line, "-"), ":");
    if (str::trim(result.name).empty())
        throw TestConfigError{lineNumber, "A section name can't be empty"};
    if (str::trim(result.name) != result.name)
        throw TestConfigError{lineNumber, "A section name can't start or end with whitespace characters"};

    result.value = str::trim(str::after(line, ":"));
    if (result.value.empty())
        readMultilineSectionValue(result, stream, multilineSectionSeparator);
    return result;
}

std::string getMultilineSectionSeparator(const std::vector<Section>& sections)
{
    auto sectionIt = ranges::find_if(sections, [](const auto& section) { return section.name == "Section separator"; });
    if (sectionIt == sections.end())
        return "---";
    return sectionIt->value;
}

}
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
    while(!stream.atEnd()){
        auto line = stream.peekLine();
        if (str::startsWith(line, "-")) {
            try{
                auto section = readSection(stream, getMultilineSectionSeparator(result));
                if (result.empty())
                    section.originalText.insert(0, sectionOuterWhitespace);
                else
                    result.back().originalText += sectionOuterWhitespace;
                sectionOuterWhitespace.clear();
                result.emplace_back(std::move(section));
            }
            catch(const TestConfigError& error){
                readingError = error;
                return result;
            }
        }
        else if (str::startsWith(line, "#") || str::trim(line).empty()){
            sectionOuterWhitespace += line;
            stream.skipLine();
        }
        else{
            readingError = TestConfigError{stream.lineNumber(), "Space outside of sections can only contain whitespace characters and comments"};
            return result;
        }
    }
    if (!result.empty())
        result.back().originalText += sectionOuterWhitespace;
    return result;
}

}