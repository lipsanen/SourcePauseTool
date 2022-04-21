#include "gtest/gtest.h"
#include <iostream>
#include "spt\srctas\framebulk.hpp"

TEST(FrameBulk, FrameBulkString)
{
    srctas::FrameBulk frameBulk;
    GTEST_ASSERT_EQ(">>>>>>|>>>>|>>>>>>>>|>|>|>|1|", frameBulk.GetFramebulkString());
}

TEST(FrameBulk, Command)
{
    srctas::FrameBulk frameBulk;
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ("", cmd);
}

TEST(FrameBulk, NegativeLetterAdd)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("tas_strafe_type -1");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(";tas_strafe_type 0", cmd);
}

TEST(FrameBulk, HugeLetterAdd)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("tas_strafe_type 9999");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(frameBulk.m_iStrafeType.m_iValue, 9);
    GTEST_ASSERT_EQ(";tas_strafe_type 9", cmd);
}

TEST(FrameBulk, QuotationMarks)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("\"tas_strafe_type\" \"3\"");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(";tas_strafe_type 3", cmd);
}

TEST(FrameBulk, MultipleCommandAddWorks)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("tas_strafe_type 3; tas_strafe_dir 1; echo XD; +tas_strafe");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(";+tas_strafe;tas_strafe_type 3;tas_strafe_dir 1;echo XD", cmd);
}

TEST(FrameBulk, EmptyCommandsAreIgnored)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand(";;;;tas_strafe_type 3;;;;tas_strafe_dir 1");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(";tas_strafe_type 3;tas_strafe_dir 1", cmd);
}

TEST(FrameBulk, RemoveKeyCodeStandardToggle)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("+tas_strafe 100");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(";+tas_strafe", cmd);
}

TEST(FrameBulk, RemoveKeyCodeOtherToggle)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("+my_custom_alias 100");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(";+my_custom_alias", cmd);
}

TEST(FrameBulk, OtherToggleInvertRemovesOldToggle)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("+my_custom_alias");
    frameBulk.ApplyCommand("-my_custom_alias");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(";-my_custom_alias", cmd);
}

TEST(FrameBulk, OtherToggleInvertRemovesSemicolon)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("+my_custom_alias");
    frameBulk.ApplyCommand("+my_custom_alias2");
    frameBulk.ApplyCommand("-my_custom_alias");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(";+my_custom_alias2;-my_custom_alias", cmd);
}

TEST(FrameBulk, ExtraSpaces)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("tas_strafe_type  \t      3");
    std::string cmd = frameBulk.GetCommand();

    GTEST_ASSERT_EQ(";tas_strafe_type 3", cmd);
}

TEST(FrameBulk, AllDeckedOutFrameBulkString)
{
    srctas::FrameBulk frameBulk;
    frameBulk.ApplyCommand("+tas_strafe");
    frameBulk.ApplyCommand("tas_strafe_type 0");
    frameBulk.ApplyCommand("tas_strafe_jumptype 1");
    frameBulk.ApplyCommand("tas_strafe_dir 2");
    frameBulk.ApplyCommand("+tas_lgagst");
    frameBulk.ApplyCommand("+tas_autojump");
    frameBulk.ApplyCommand("+forward");
    frameBulk.ApplyCommand("+moveleft");
    frameBulk.ApplyCommand("+back");
    frameBulk.ApplyCommand("+moveright");
    frameBulk.ApplyCommand("+jump");
    frameBulk.ApplyCommand("+duck");
    frameBulk.ApplyCommand("+use");
    frameBulk.ApplyCommand("+attack");
    frameBulk.ApplyCommand("+attack2");
    frameBulk.ApplyCommand("+reload");
    frameBulk.ApplyCommand("+walk");
    frameBulk.ApplyCommand("+speed");
    frameBulk.ApplyCommand("_y_spt_setpitch 5");
    frameBulk.ApplyCommand("_y_spt_setyaw 2.5");
    frameBulk.ApplyCommand("tas_strafe_yaw 40.5");
    frameBulk.ApplyCommand("echo \"Hello, world!\"");
    std::string cmd = frameBulk.GetCommand();
    std::cout << cmd << std::endl;

    GTEST_ASSERT_EQ("s012lj|wasd|jdu12rws|5|2.5|40.5|1|echo \"Hello, world!\"", frameBulk.GetFramebulkString());
}

TEST(FrameBulk, ParseEmpty)
{
    std::string input = ">>>>>>|>>>>|>>>>>>>>|>|>|>|1|";
    srctas::FrameBulkError error;
    auto bulk = srctas::FrameBulk::Parse(input, error);
    GTEST_ASSERT_EQ(false, error.m_bError);
    auto output = bulk.GetFramebulkString();
    GTEST_ASSERT_EQ(input, output);
}

TEST(FrameBulk, ParseEmptyMissingChars)
{
    std::string input = "|||>|>|>|1|";
    srctas::FrameBulkError error;
    auto bulk = srctas::FrameBulk::Parse(input, error);
    GTEST_ASSERT_EQ(false, error.m_bError);
    auto output = bulk.GetFramebulkString();
    GTEST_ASSERT_EQ(">>>>>>|>>>>|>>>>>>>>|>|>|>|1|", output);
}

TEST(FrameBulk, ParseSomeMissingChars)
{
    std::string input = "s012|wa|jdu12rw|>|>|>|1|";
    srctas::FrameBulkError error;
    auto bulk = srctas::FrameBulk::Parse(input, error);
    GTEST_ASSERT_EQ(false, error.m_bError);
    auto output = bulk.GetFramebulkString();
    GTEST_ASSERT_EQ("s012>>|wa>>|jdu12rw>|>|>|>|1|", output);
}

TEST(FrameBulk, ParseFull)
{
    std::string input = "s012lj|wasd|jdu12rws|0|-125.5|2.5|128|echo \"Hello, world!\"";
    srctas::FrameBulkError error;
    auto bulk = srctas::FrameBulk::Parse(input, error);
    if(error.m_bError)
    {
        std::cout << error.m_sMessage << std::endl;
    }
    GTEST_ASSERT_EQ(false, error.m_bError);
    GTEST_ASSERT_EQ(bulk.m_sCommands, "echo \"Hello, world!\"");
    auto output = bulk.GetFramebulkString();
    GTEST_ASSERT_EQ(input, output);
}

TEST(FrameBulk, ParseBadNumber)
{
    std::string input = ">|>|>|a|>|>|>|";
    srctas::FrameBulkError error;
    auto bulk = srctas::FrameBulk::Parse(input, error);
    if(error.m_bError)
    {
        std::cout << error.m_sMessage << std::endl;
    }
    GTEST_ASSERT_EQ(true, error.m_bError);

    input = ">|>|>|>|a|>|>|";
    bulk = srctas::FrameBulk::Parse(input, error);
    if(error.m_bError)
    {
        std::cout << error.m_sMessage << std::endl;
    }
    GTEST_ASSERT_EQ(true, error.m_bError);

    input = ">|>|>|>|>|a|>|";
    bulk = srctas::FrameBulk::Parse(input, error);
    if(error.m_bError)
    {
        std::cout << error.m_sMessage << std::endl;
    }
    GTEST_ASSERT_EQ(true, error.m_bError);

    input = ">|>|>|>|>|>|a|";
    bulk = srctas::FrameBulk::Parse(input, error);
    if(error.m_bError)
    {
        std::cout << error.m_sMessage << std::endl;
    }
    GTEST_ASSERT_EQ(true, error.m_bError);
}

TEST(FrameBulk, FrameBulkHalfMissing)
{
    std::string input = "s012|wa|jd";
    srctas::FrameBulkError error;
    auto bulk = srctas::FrameBulk::Parse(input, error);
    std::cout << error.m_sMessage << std::endl;
    GTEST_ASSERT_EQ(true, error.m_bError);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}