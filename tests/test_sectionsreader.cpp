#include <sectionsreader.h>
#include <gtest/gtest.h>
#include <sstream>
#include <functional>

bool operator==(const Section& lhs, const Section& rhs)
{
    return lhs.name == rhs.name &&
           lhs.value == rhs.value;
}

void PrintTo(const Section& section, std::ostream *os)
{
  *os << section.name << "=" << section.value << std::endl;
}

void testSectionReader(const std::string& input,
                       const std::vector<Section>& expectedSections,
                       const std::vector<std::string>& rawSectionsList = {})
{
    auto stream = std::istringstream{input};
    auto sections = readSections(stream, rawSectionsList);
    EXPECT_EQ(sections, expectedSections);
}

void testSectionValueReader(const std::string& input,
                            const std::string& sectionName,
                            const std::string& expectedValue,
                            bool isRaw = false)
{
    auto stream = std::istringstream{input};
    auto sectionValue = readSectionValue(stream, sectionName, isRaw);
    EXPECT_EQ(sectionValue, expectedValue);
}


template<typename ExceptionType>
void assert_exception(std::function<void()> throwingCode, std::function<void(const ExceptionType&)> exceptionContentChecker)
{
    try{
        throwingCode();
        FAIL() << "exception wasn't thrown!";
    }
    catch(const ExceptionType& e){
        exceptionContentChecker(e);
    }
    catch(...){
        FAIL() << "Unexpected exception was thrown";
    }
}

TEST(SectionsReader, Basic)
{
    testSectionReader(
        "-Name:foo\n"
        "test\n"
        "\n"
        "-Value:\n"
        "bar\n",
        {
            {"Name", "foo\ntest\n\n"},
            {"Value", "bar\n"}
        });
}

TEST(SectionsReader, EmptySection)
{
    testSectionReader(
        "-Name:foo\n"
        "test\n"
        "\n"
        "-Empty:\n"
        "-Value:\n"
        "bar\n",
        {
            {"Name", "foo\ntest\n\n"},
            {"Empty", ""},
            {"Value", "bar\n"}
        });
}

TEST(SectionsReader, SingleEmptySection)
{
    testSectionReader(
        "-Empty:",
        {
            {"Empty", ""}
        });
}

TEST(SectionsReader, NoSections)
{
    testSectionReader(
        "Empty:"
        "Value",
        {});
}

TEST(SectionsReader, NamelessSection)
{
    assert_exception<std::runtime_error>([]{
        auto input = std::string{"-Name: foo\n-:\n-Value:foo\n"};
        auto stream = std::istringstream{input};
        readSections(stream);
    },
    [](const std::runtime_error& e){
        ASSERT_EQ(std::string{e.what()}, "line#1: Section must have a name");
    });
}

TEST(SectionsReader, SectionWithoutDelimiter)
{
    assert_exception<std::runtime_error>([]{
        auto input = std::string{"-Name: foo\n-\n-Value:foo\n"};
        auto stream = std::istringstream{input};
        readSections(stream);
    },
    [](const std::runtime_error& e){
        ASSERT_EQ(std::string{e.what()}, "line#1: Section's first line must contain a name delimiter ':'");
    });
}



TEST(SectionsReader, WithComment)
{
    testSectionReader(
        "-Name:foo\n"
        "#this is Name section\n"
        "test\n"
        "\n"
        "-Value:\n"
        "bar\n",
        {
            {"Name", "foo\ntest\n\n"},
            {"Value", "bar\n"}
        });
}

TEST(SectionsReader, RawSectionWithComment)
{
    testSectionReader(
        "-Write test.txt:foo\n"
        "#this line should be in test.txt\n"
        "\n"
        "-Value:\n"
        "bar\n",
        {
            {"Write test.txt", "foo\n#this line should be in test.txt\n\n"},
            {"Value", "bar\n"}
        },
        {"Write"});
}

TEST(SectionsReader, TextBeforeSections)
{
    testSectionReader(
        "Loren ipsum\n"
        "# some comment\n"
        "\n"
        "-Name:foo\n"
        "test\n"
        "\n"
        "-Value:\n"
        "bar\n",
        {
            {"Name", "foo\ntest\n\n"},
            {"Value", "bar\n"}
        });
}

TEST(SectionValueReader, Basic)
{
    auto input = "-Name:foo\n"
                 "test\n"
                 "\n"
                 "-Value:\n"
                 "bar\n";
    testSectionValueReader(
        input, "Name", "foo\ntest\n\n");
    testSectionValueReader(
        input, "Value", "bar\n");
}

TEST(SectionsValueReader, EmptySection)
{
    auto input = "-Name:foo\n"
                 "test\n"
                 "\n"
                 "-Empty:\n"
                 "-Value:\n"
                 "bar\n";
    testSectionValueReader(
        input, "Name", "foo\ntest\n\n");
    testSectionValueReader(
        input, "Empty", "");
    testSectionValueReader(
        input, "Value", "bar\n");
}

TEST(SectionsValueReader, SingleEmptySection)
{
    testSectionValueReader(
        "-Empty:",
        "Empty", "");

}

TEST(SectionsValueReader, NoSections)
{
    testSectionValueReader(
        "Empty:"
        "Value",
        "Empty", "");
    testSectionValueReader(
        "Empty:"
        "Value",
        "Value", "");
}

TEST(SectionsValueReader, WithComment)
{
    auto input = "-Name:foo\n"
                 "#this is Name section\n"
                 "test\n"
                 "\n"
                 "-Value:\n"
                 "bar\n";
    testSectionValueReader(input, "Name", "foo\ntest\n\n");
    testSectionValueReader(input, "Value", "bar\n");
}

TEST(SectionsValueReader, RawSectionWithComment)
{
    auto input = "-Name:foo\n"
                 "#this line should be in section\n"
                 "\n"
                 "-Value:\n"
                 "bar\n";
    testSectionValueReader(input, "Name", "foo\n#this line should be in section\n\n", true);
    testSectionValueReader(input, "Value", "bar\n");
}


