#include "assert_exception.h"
#include <errors.h>
#include <useractionformatparser.h>
#include <gtest/gtest.h>

using namespace lunchtoast;

TEST(UserActionFormatParser, SingleParam)
{
    auto userActionFormat = makeUserActionFormat("%1");
    EXPECT_EQ(userActionFormat.formatRegex, R"((.+))");
    EXPECT_EQ(userActionFormat.paramsOrder, std::vector{1});
}

TEST(UserActionFormatParser, SingleParam2)
{
    auto userActionFormat = makeUserActionFormat("Hello world %1");
    EXPECT_EQ(userActionFormat.formatRegex, R"(Hello world\s+(.+))");
    EXPECT_EQ(userActionFormat.paramsOrder, std::vector{1});
}

TEST(UserActionFormatParser, SingleParam3)
{
    auto userActionFormat = makeUserActionFormat("%1 Hello world");
    EXPECT_EQ(userActionFormat.formatRegex, R"((.+?)\s+Hello world)");
    EXPECT_EQ(userActionFormat.paramsOrder, std::vector{1});
}

TEST(UserActionFormatParser, SingleParam4)
{
    auto userActionFormat = makeUserActionFormat("Hello%1world");
    EXPECT_EQ(userActionFormat.formatRegex, R"(Hello(.+?)world)");
    EXPECT_EQ(userActionFormat.paramsOrder, std::vector{1});
}

TEST(UserActionFormatParser, MultipleParams)
{
    auto userActionFormat = makeUserActionFormat("%3 %1 %2");
    EXPECT_EQ(userActionFormat.formatRegex, R"((.+?)\s+(.+?)\s+(.+))");
    EXPECT_EQ(userActionFormat.paramsOrder, (std::vector{3, 1, 2}));
}

TEST(UserActionFormatParser, MultipleParams2)
{
    auto userActionFormat = makeUserActionFormat("%3 Hello %1 world %2");
    EXPECT_EQ(userActionFormat.formatRegex, R"((.+?)\s+Hello\s+(.+?)\s+world\s+(.+))");
    EXPECT_EQ(userActionFormat.paramsOrder, (std::vector{3, 1, 2}));
}

TEST(UserActionFormatParser, MultipleParams3)
{
    auto userActionFormat = makeUserActionFormat("%3Hello%1 world %2");
    EXPECT_EQ(userActionFormat.formatRegex, R"((.+?)Hello(.+?)\s+world\s+(.+))");
    EXPECT_EQ(userActionFormat.paramsOrder, (std::vector{3, 1, 2}));
}

TEST(UserActionFormatParser, ErrorSameParameters)
{
    assert_exception<lunchtoast::ActionFormatError>(
            []
            {
                makeUserActionFormat("%3Hello%1 world %1");
            },
            [](const auto& e)
            {
                ASSERT_EQ(
                        std::string{e.what()},
                        "Action format '%3Hello%1 world %1' is invalid: encountered multiple parameters %1, format "
                        "parameters indices must be unique");
            });
}

TEST(UserActionFormatParser, ErrorMissingParameters)
{
    assert_exception<lunchtoast::ActionFormatError>(
            []
            {
                makeUserActionFormat("%3Hello%4 world %1");
            },
            [](const auto& e)
            {
                ASSERT_EQ(
                        std::string{e.what()},
                        "Action format '%3Hello%4 world %1' is invalid: format parameters indices must form a "
                        "continuous range starting with 1");
            });
}

TEST(UserActionFormatParser, ErrorMissingParameters2)
{
    assert_exception<lunchtoast::ActionFormatError>(
            []
            {
                makeUserActionFormat("%2Hello%3 world %4");
            },
            [](const auto& e)
            {
                ASSERT_EQ(
                        std::string{e.what()},
                        "Action format '%2Hello%3 world %4' is invalid: format parameters indices must form a "
                        "continuous range starting with 1");
            });
}