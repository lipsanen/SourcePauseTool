#include "gtest/gtest.h"
#include "spt/srctas/cmd_parsing.hpp"

TEST(CmdParsing, KeycodePlusCommand)
{
    const char* variable = "+attack 100";
    char command[64];
    strcpy(command, variable);
    srctas::RemoveKeyCode(command);
    GTEST_ASSERT_EQ(strcmp(command, "+attack"), 0);
}

TEST(CmdParsing, KeycodeMinusCommand)
{
    const char* variable = "-attack 1";
    char command[64];
    strcpy(command, variable);
    srctas::RemoveKeyCode(command);
    GTEST_ASSERT_EQ(strcmp(command, "-attack"), 0);
}

TEST(CmdParsing, SpacesAreHandled)
{
    const char* variable = " -attack 1";
    char command[64];
    strcpy(command, variable);
    srctas::RemoveKeyCode(command);
    GTEST_ASSERT_EQ(strcmp(command, " -attack"), 0);
}

TEST(CmdParsing, OtherCommandsUnaffected)
{
    const char* variable = " echo 1";
    char command[64];
    strcpy(command, variable);
    srctas::RemoveKeyCode(command);
    GTEST_ASSERT_EQ(strcmp(command, variable), 0);
}