#include "assert_exception.h"
#include <errors.h>
#include <sectionsreader.h>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <functional>
#include <sstream>

namespace lunchtoast {
bool operator==(const lunchtoast::Section& lhs, const lunchtoast::Section& rhs)
{
    return lhs.name == rhs.name && lhs.value == rhs.value && lhs.originalText == rhs.originalText;
}

void PrintTo(const lunchtoast::Section& section, std::ostream* os)
{
    *os << fmt::format(
            R"([
  .name = {}
  .value = {}
  .originalText = {}
]
)",
            section.name,
            section.value,
            section.originalText);
}

} //namespace lunchtoast

void testSectionReader(const std::string& input, const std::vector<lunchtoast::Section>& expectedSections)
{
    auto stream = std::istringstream{input};
    auto sections = lunchtoast::readSections(stream);
    EXPECT_EQ(sections, expectedSections);
}

TEST(SectionsReader, Basic)
{
    testSectionReader(
            "-Name:foo\n"
            "#TEST COMMENT\n"
            "-Value:\n"
            "bar\n"
            "---\n",
            {{"Name", "foo", "-Name:foo\n#TEST COMMENT\n"}, {"Value", "bar", "-Value:\nbar\n---\n"}});
}

TEST(SectionsReader, BasicCRLF)
{
    testSectionReader(
            "-Name:foo\r\n"
            "#TEST COMMENT\r\n"
            "-Value:\r\n"
            "bar\r\n"
            "---\r\n",
            {{"Name", "foo", "-Name:foo\n#TEST COMMENT\n"}, {"Value", "bar", "-Value:\nbar\n---\n"}});
}

TEST(SectionsReader, BasicCR)
{
    testSectionReader(
            "-Name:foo\r"
            "#TEST COMMENT\r"
            "-Value:\r"
            "bar\r"
            "---\r",
            {{"Name", "foo", "-Name:foo\n#TEST COMMENT\n"}, {"Value", "bar", "-Value:\nbar\n---\n"}});
}

TEST(SectionsReader, Comments)
{
    testSectionReader(
            "COMMENT 1\n"
            "-Name:foo\n"
            "#COMMENT 2\n"
            "//COMMENT 2-1\n"
            "-Value:\n"
            "#NOT A COMMENT\n"
            "bar\n"
            "---\n"
            "COMMENT 3\n",
            {{"Name", "foo", "COMMENT 1\n-Name:foo\n#COMMENT 2\n//COMMENT 2-1\n"},
             {"Value", "#NOT A COMMENT\nbar", "-Value:\n#NOT A COMMENT\nbar\n---\nCOMMENT 3\n"}});
}

TEST(SectionsReader, MultilineSection)
{
    testSectionReader(
            "-Write test.txt:\n"
            "foo\n"
            "---\n",
            {{"Write test.txt", "foo", "-Write test.txt:\nfoo\n---\n"}});
}

TEST(SectionsReader, MultilineSection2)
{
    testSectionReader(
            "-Write test.txt:\n"
            "foo\n"
            "\n"
            "---\n",
            {{"Write test.txt", "foo\n", "-Write test.txt:\nfoo\n\n---\n"}});
}

TEST(SectionsReader, MultilineSectionCustomSeparator)
{
    testSectionReader(
            "-Section separator: ---lunchtoast\n"
            "-Write test.txt:\n"
            "foo\n"
            "---lunchtoast\n",
            {{"Section separator", "---lunchtoast", "-Section separator: ---lunchtoast\n"},
             {"Write test.txt", "foo", "-Write test.txt:\nfoo\n---lunchtoast\n"}});
}

TEST(SectionsReader, MultilineSectionEmpty)
{
    testSectionReader(
            "-Write test.txt:\n"
            "---\n",
            {{"Write test.txt", "", "-Write test.txt:\n---\n"}});
}

TEST(SectionsReader, SingleLineSection)
{
    testSectionReader("-Name:foo", {{"Name", "foo", "-Name:foo"}});
}

TEST(SectionsReader, EmptySection)
{
    testSectionReader("-Name", {{"Name", "", "-Name"}});
}

TEST(SectionsReader, EmptySection2)
{
    testSectionReader("-Name and Surname\n", {{"Name and Surname", "", "-Name and Surname\n"}});
}

TEST(SectionsReader, NameWithColon)
{
    testSectionReader("-Name`00:01`: value", {{"Name`00:01`", "value", "-Name`00:01`: value"}});
}

TEST(SectionsReader, NameWithColon2)
{
    testSectionReader("-Name\'00:01\': value", {{"Name\'00:01\'", "value", "-Name\'00:01\': value"}});
}

TEST(SectionsReader, NameWithColon3)
{
    testSectionReader("-Name\"00:01\": value", {{"Name\"00:01\"", "value", "-Name\"00:01\": value"}});
}

TEST(SectionsReader, EmptySectionWithColon)
{
    testSectionReader("-Name`00:01`", {{"Name`00:01`", "", "-Name`00:01`"}});
}

TEST(SectionsReader, ValueWithColon)
{
    testSectionReader("-Name: localhost:8080", {{"Name", "localhost:8080", "-Name: localhost:8080"}});
}

TEST(SectionsReader, ErrorEmptySectionName)
{
    assert_exception<lunchtoast::TestConfigError>(
            []
            {
                testSectionReader(
                        "- :foo\n"
                        "-Value: bar",
                        {});
            },
            [](const auto& e)
            {
                ASSERT_EQ(std::string{e.what()}, "line 1: A section name can't be empty");
            });
}

TEST(SectionsReader, ErrorSectionNameStartsWithWhitespace)
{
    assert_exception<lunchtoast::TestConfigError>(
            []
            {
                testSectionReader(
                        "- Test:foo\n"
                        "-Value: bar",
                        {});
            },
            [](const auto& e)
            {
                ASSERT_EQ(
                        std::string{e.what()},
                        "line 1: A section name can't start or end with whitespace characters");
            });
}

TEST(SectionsReader, ErrorSectionNameEndsWithWhitespace)
{
    assert_exception<lunchtoast::TestConfigError>(
            []
            {
                testSectionReader(
                        "- Test:foo\n"
                        "-Value: bar",
                        {});
            },
            [](const auto& e)
            {
                ASSERT_EQ(
                        std::string{e.what()},
                        "line 1: A section name can't start or end with whitespace characters");
            });
}

TEST(SectionsReader, ErrorMultilineSectionWithoutSeparator)
{
    assert_exception<lunchtoast::TestConfigError>(
            []
            {
                testSectionReader(
                        "-Test:\n"
                        "-Foo\n"
                        "-Value: bar",
                        {});
            },
            [](const auto& e)
            {
                ASSERT_EQ(std::string{e.what()}, "line 1: A multiline section must be closed with '---' separator");
            });
}

TEST(SectionsReader, ErrorMultilineSectionWithoutSeparator2)
{
    assert_exception<lunchtoast::TestConfigError>(
            []
            {
                testSectionReader(
                        "-Test: foo\n"
                        "-Foo:   \n"
                        "-Value: bar",
                        {});
            },
            [](const auto& e)
            {
                ASSERT_EQ(std::string{e.what()}, "line 2: A multiline section must be closed with '---' separator");
            });
}

TEST(SectionsReader, ErrorMultilineSectionWithoutCustomSeparator)
{
    assert_exception<lunchtoast::TestConfigError>(
            []
            {
                testSectionReader(
                        "-Section separator: ---lunchtoast\n"
                        "-Test:\n"
                        "-Foo\n"
                        "-Value: bar",
                        {});
            },
            [](const auto& e)
            {
                ASSERT_EQ(
                        std::string{e.what()},
                        "line 2: A multiline section must be closed with '---lunchtoast' separator");
            });
}

TEST(SectionsReader, ErrorMultilineSectionSeparatorStartsWithWhitespace)
{
    assert_exception<lunchtoast::TestConfigError>(
            []
            {
                testSectionReader(
                        "-Test: foo\n"
                        "-Foo:   \n"
                        "  ---\n"
                        "-Value: bar",
                        {});
            },
            [](const auto& e)
            {
                ASSERT_EQ(
                        std::string{e.what()},
                        "line 3: A multiline section separator must be placed at the start of a line");
            });
}
