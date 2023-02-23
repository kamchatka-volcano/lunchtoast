#include <utils.h>
#include <fmt/format.h>
#include <gtest/gtest.h>
#include <functional>
#include <sstream>

TEST(Utils, SplitCommand)
{
    auto parts = lunchtoast::splitCommand("command -param \"hello world\"");
    ASSERT_EQ(parts, (std::vector<std::string_view>{"command", "-param", "hello world"}));
}

TEST(Utils, SplitCommand2)
{
    auto parts = lunchtoast::splitCommand("command -param \"hello world\" -param2 \"hello world 2\"");
    ASSERT_EQ(parts, (std::vector<std::string_view>{"command", "-param", "hello world", "-param2", "hello world 2"}));
}

TEST(Utils, SplitCommand3)
{
    auto parts = lunchtoast::splitCommand("command -param=\"hello world\" -param2=\"hello world 2\"");
    ASSERT_EQ(parts, (std::vector<std::string_view>{"command", "-param=\"hello world\"", "-param2=\"hello world 2\""}));
}

TEST(Utils, SplitCommand4)
{
    auto parts = lunchtoast::splitCommand("\"hello world\" command");
    ASSERT_EQ(parts, (std::vector<std::string_view>{"hello world", "command"}));
}

TEST(Utils, SplitCommand5)
{
    auto parts = lunchtoast::splitCommand("\"  hello world  \" command");
    ASSERT_EQ(parts, (std::vector<std::string_view>{"  hello world  ", "command"}));
}

TEST(Utils, SplitCommand6)
{
    auto parts = lunchtoast::splitCommand("\" hello world \" --param2 \"hello world 2\" -param3=\"hello world 3\"");
    ASSERT_EQ(
            parts,
            (std::vector<std::string_view>{" hello world ", "--param2", "hello world 2", "-param3=\"hello world 3\""}));
}

TEST(Utils, SplitCommandNoWhitespace)
{
    auto parts = lunchtoast::splitCommand("command");
    ASSERT_EQ(parts, (std::vector<std::string_view>{"command"}));
}

TEST(Utils, SplitCommandEmpty)
{
    auto parts = lunchtoast::splitCommand("");
    ASSERT_EQ(parts, (std::vector<std::string_view>{""}));
}

TEST(Utils, SplitCommandUnclosedString)
{
    auto parts = lunchtoast::splitCommand("command -param \"hello world");
    ASSERT_EQ(parts, (std::vector<std::string_view>{}));
}

TEST(Utils, SplitCommandUnclosedString2)
{
    auto parts = lunchtoast::splitCommand("command -param \"");
    ASSERT_EQ(parts, (std::vector<std::string_view>{}));
}