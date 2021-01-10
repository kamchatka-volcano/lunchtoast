#include <sectionsreader.h>
#include <gtest/gtest.h>
#include <sstream>
#include <functional>

bool operator==(const Section& lhs, const Section& rhs)
{
    return lhs.name == rhs.name &&
           lhs.value == rhs.value &&
           lhs.isVoid == rhs.isVoid;
}

bool operator==(const RestorableSection& lhs, const RestorableSection& rhs)
{
    return lhs.name == rhs.name &&
           lhs.value == rhs.value &&
           lhs.isVoid == rhs.isVoid &&
           lhs.originalText == rhs.originalText &&
           lhs.isComment == rhs.isComment;
}


void PrintTo(const Section& section, std::ostream *os)
{
  *os << section.name << "=" << section.value << " (isVoid:" << section.isVoid << ")" <<std::endl;
}

void PrintTo(const RestorableSection& section, std::ostream *os)
{
  *os << section.name << "=" << section.value << "|"
      << section.originalText << " (isComment" << section.isComment << ")" << std::endl;
}

void testSectionReader(const std::string& input,
                       const std::vector<Section>& expectedSections,
                       const std::vector<RawSectionSpecifier>& rawSectionsList = {})
{
    auto stream = std::istringstream{input};
    auto sections = readSections(stream, rawSectionsList);
    EXPECT_EQ(sections, expectedSections);
}

void testRawSectionReader(const std::string& input,
                          const std::vector<RestorableSection>& expectedSections,
                          const std::vector<RawSectionSpecifier>& rawSectionsList = {})
{
    auto stream = std::istringstream{input};
    auto sections = readRestorableSections(stream, rawSectionsList);
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

TEST(RawSectionsReader, Basic)
{
    testRawSectionReader(
        "-Name:foo\n"
        "test\n"
        "\n"
        "-Value:\n"
        "bar\n",
        {
            {"Name", "foo\ntest\n\n", "-Name:foo\ntest\n\n"},
            {"Value", "bar\n", "-Value:\nbar\n"}
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

TEST(RawSectionsReader, EmptySection)
{
    testRawSectionReader(
        "-Name:foo\n"
        "test\n"
        "\n"
        "-Empty:\n"
        "-Value:\n"
        "bar\n",
        {
            {"Name", "foo\ntest\n\n", "-Name:foo\ntest\n\n"},
            {"Empty", "", "-Empty:\n"},
            {"Value", "bar\n", "-Value:\nbar\n"}
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

TEST(RawSectionsReader, SingleEmptySection)
{
    testRawSectionReader(
        "-Empty:",
        {
            {"Empty", "", "-Empty:\n"}
        });
}

TEST(SectionsReader, NoSections)
{
    testSectionReader(
        "Empty:"
        "Value",
        {});
}

TEST(RawSectionsReader, NoSections)
{
    testRawSectionReader(
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
        ASSERT_EQ(std::string{e.what()}, "line#2: Section must have a name");
    });
}

TEST(RawSectionsReader, NamelessSection)
{
    assert_exception<std::runtime_error>([]{
        auto input = std::string{"-Name: foo\n-:\n-Value:foo\n"};
        auto stream = std::istringstream{input};
        readRestorableSections(stream);
    },
    [](const std::runtime_error& e){
        ASSERT_EQ(std::string{e.what()}, "line#2: Section must have a name");
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
        ASSERT_EQ(std::string{e.what()}, "line#2: Section's first line must contain a name delimiter ':'");
    });
}

TEST(RawSectionsReader, SectionWithoutDelimiter)
{
    assert_exception<std::runtime_error>([]{
        auto input = std::string{"-Name: foo\n-\n-Value:foo\n"};
        auto stream = std::istringstream{input};
        readRestorableSections(stream);
    },
    [](const std::runtime_error& e){
        ASSERT_EQ(std::string{e.what()}, "line#2: Section's first line must contain a name delimiter ':'");
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

TEST(RawSectionsReader, WithComment)
{
    testRawSectionReader(
        "-Name:foo\n"
        "#this is Name section\n"
        "test\n"
        "\n"
        "-Value:\n"
        "bar\n",
        {
            {"Name", "foo\ntest\n\n",
             "-Name:foo\n"
             "#this is Name section\n"
             "test\n"
             "\n"},
            {"Value", "bar\n",
             "-Value:\n"
             "bar\n"}
        });
}

TEST(RawSectionsReader, WithComments)
{
    testRawSectionReader(
        "#first comment\n"
        "#line2\n"
        "#line3\n"
        "-Name:foo\n"
        "#this is Name section\n"
        "test\n"
        "\n"
        "-Value:\n"
        "bar\n"
        "#another comment",
        {
            {"", "",
             "#first comment\n"
             "#line2\n"
             "#line3\n", true},
            {"Name", "foo\ntest\n\n",
             "-Name:foo\n"
             "#this is Name section\n"
             "test\n"
             "\n"},
            {"Value", "bar\n",
             "-Value:\n"
             "bar\n"
             "#another comment\n"}
        });
}

TEST(SectionsReader, RawSectionWithComment)
{
    testSectionReader(
        "-Write test.txt:foo\n"
        "#this line should be in test.txt\n"
        "-this shoudln't be a new section:\n"
        "\n"
        "---\n"
        "-Value:\n"
        "bar\n",
        {
            {"Write test.txt", "foo\n#this line should be in test.txt\n-this shoudln't be a new section:\n\n"},
            {"Value", "bar\n"}
        },
        {RawSectionSpecifier{"Write", "---"}});
}


TEST(RawSectionsReader, RawSectionWithComment)
{
    testRawSectionReader(
        "-Write test.txt:foo\n"
        "#this line should be in test.txt\n"
        "-this shoudln't be a new section:\n"
        "\n"
        "---\n"
        "-Value:\n"
        "bar\n",
        {
            {"Write test.txt", "foo\n#this line should be in test.txt\n-this shoudln't be a new section:\n\n",
             "-Write test.txt:foo\n"
             "#this line should be in test.txt\n"
             "-this shoudln't be a new section:\n"
             "\n"},
            {"", "", "---\n", true},
            {"Value", "bar\n",
             "-Value:\n"
             "bar\n"}
        },
        {RawSectionSpecifier{"Write", "---"}});
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

TEST(RawSectionsReader, TextBeforeSections)
{
    testRawSectionReader(
        "Loren ipsum\n"
        "# some comment\n"
        "\n"
        "-Name:foo\n"
        "test\n"
        "\n"
        "-Value:\n"
        "bar\n",
        {
            {"","", "# some comment\n", true},
            {"Name", "foo\ntest\n\n",
             "-Name:foo\n"
             "test\n"
             "\n"},
            {"Value", "bar\n",
             "-Value:\n"
             "bar\n"}
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


