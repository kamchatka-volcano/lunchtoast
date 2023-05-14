#include "assert_exception.h"
#include "errors.h"
#include <utils.h>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <functional>
#include <sstream>

TEST(Utils, SplitCommand)
{
    auto parts = lunchtoast::splitCommand("command -param \"hello world\"");
    ASSERT_EQ(parts, (std::vector<std::string>{"command", "-param", "hello world"}));
}

TEST(Utils, SplitCommand2)
{
    auto parts = lunchtoast::splitCommand("command -param \"hello world\" -param2 \"hello world 2\"");
    ASSERT_EQ(parts, (std::vector<std::string>{"command", "-param", "hello world", "-param2", "hello world 2"}));
}

TEST(Utils, SplitCommand3)
{
    auto parts = lunchtoast::splitCommand("command -param=\"hello world\" -param2=\"hello world 2\"");
    ASSERT_EQ(parts, (std::vector<std::string>{"command", "-param=hello world", "-param2=hello world 2"}));
}

TEST(Utils, SplitCommand4)
{
    auto parts = lunchtoast::splitCommand("\"hello world\" command");
    ASSERT_EQ(parts, (std::vector<std::string>{"hello world", "command"}));
}

TEST(Utils, SplitCommand5)
{
    auto parts = lunchtoast::splitCommand("\"  hello world  \" command");
    ASSERT_EQ(parts, (std::vector<std::string>{"  hello world  ", "command"}));
}

TEST(Utils, SplitCommand6)
{
    auto parts = lunchtoast::splitCommand("\" hello world \" --param2 \"hello world 2\" -param3=\"hello world 3\"");
    ASSERT_EQ(parts, (std::vector<std::string>{" hello world ", "--param2", "hello world 2", "-param3=hello world 3"}));
}

TEST(Utils, SplitCommand7)
{
    auto parts = lunchtoast::splitCommand("' hello world ' --param2 'hello world 2' -param3='hello world 3'");
    ASSERT_EQ(parts, (std::vector<std::string>{" hello world ", "--param2", "hello world 2", "-param3=hello world 3"}));
}

TEST(Utils, SplitCommand8)
{
    auto parts = lunchtoast::splitCommand("` hello world ` --param2 `hello world 2` -param3=`hello world 3`");
    ASSERT_EQ(parts, (std::vector<std::string>{" hello world ", "--param2", "hello world 2", "-param3=hello world 3"}));
}

TEST(Utils, SplitCommand9)
{
    auto parts = lunchtoast::splitCommand("-param=\"hello\" hello_world -param2=world");
    ASSERT_EQ(parts, (std::vector<std::string>{"-param=hello", "hello_world", "-param2=world"}));
}

TEST(Utils, SplitCommandNoWhitespace)
{
    auto parts = lunchtoast::splitCommand("command");
    ASSERT_EQ(parts, (std::vector<std::string>{"command"}));
}

TEST(Utils, SplitCommandEmpty)
{
    auto parts = lunchtoast::splitCommand("");
    ASSERT_EQ(parts, (std::vector<std::string>{}));
}

TEST(Utils, SplitCommandUnclosedString)
{
    assert_exception<lunchtoast::TestConfigError>(
            []
            {
                [[maybe_unused]] auto parts = lunchtoast::splitCommand("command -param \"hello world");
            },
            [](const auto& e)
            {
                ASSERT_EQ(
                        std::string{e.what()},
                        "Command 'command -param \"hello world' has an unclosed quotation mark");
            });
}

TEST(Utils, SplitCommandUnclosedString2)
{
    assert_exception<lunchtoast::TestConfigError>(
            []
            {
                [[maybe_unused]] auto parts = lunchtoast::splitCommand("command -param \"");
            },
            [](const auto& e)
            {
                ASSERT_EQ(std::string{e.what()}, "Command 'command -param \"' has an unclosed quotation mark");
            });
}

TEST(Utils, ReadInputParams)
{
    const auto sections = lunchtoast::readInputParamSections(R"(
#foo:
Hello world
#bar:
Hello moon)");
    const auto expectedSections =
            std::unordered_map<std::string, std::string>{{"foo", "Hello world"}, {"bar", "Hello moon"}};
    EXPECT_EQ(sections, expectedSections);
}

TEST(Utils, ReadInputParams2)
{
    const auto sections = lunchtoast::readInputParamSections(R"(
#foo:
Hello world

#bar:
Hello moon
)");
    const auto expectedSections =
            std::unordered_map<std::string, std::string>{{"foo", "Hello world\n"}, {"bar", "Hello moon\n"}};
    EXPECT_EQ(sections, expectedSections);
}

TEST(Utils, ReadInputParams3)
{
    const auto sections = lunchtoast::readInputParamSections(R"(
#foo:
#bar:
Hello moon)");
    const auto expectedSections = std::unordered_map<std::string, std::string>{{"foo", ""}, {"bar", "Hello moon"}};
    EXPECT_EQ(sections, expectedSections);
}

TEST(Utils, ReadInputParams4)
{
    const auto sections = lunchtoast::readInputParamSections(R"(
#foo:

Hello world
#bar:

Hello moon
)");
    const auto expectedSections =
            std::unordered_map<std::string, std::string>{{"foo", "\nHello world"}, {"bar", "\nHello moon\n"}};
    EXPECT_EQ(sections, expectedSections);
}

TEST(Utils, ReadInputParams5)
{
    const auto sections = lunchtoast::readInputParamSections(R"(
Hello world
Hello moon
)");
    const auto expectedSections = std::unordered_map<std::string, std::string>{};
    EXPECT_EQ(sections, expectedSections);
}

TEST(Utils, ReadInputParams6)
{
    const auto sections = lunchtoast::readInputParamSections(R"()");
    const auto expectedSections = std::unordered_map<std::string, std::string>{};
    EXPECT_EQ(sections, expectedSections);
}