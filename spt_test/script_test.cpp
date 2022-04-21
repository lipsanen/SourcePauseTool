#include "gtest/gtest.h"
#include "spt\srctas\script.hpp"
#include <iostream>

TEST(Script, NullFilepath)
{
    srctas::Script script;
    auto output = srctas::Script::ParseFrom(nullptr, &script);
    std::cout << output.m_sMessage << std::endl;
    GTEST_ASSERT_EQ(true, output.m_bError);
}

TEST(Script, InvalidFilepath)
{
    srctas::Script script;
    auto output = srctas::Script::ParseFrom("./flyingspaghettimonster.src2tas", &script);
    std::cout << output.m_sMessage << std::endl;
    GTEST_ASSERT_EQ(true, output.m_bError);
}

TEST(Script, Minimal)
{
    srctas::Script script;
    auto output = srctas::Script::ParseFrom("./test_scripts/empty.src2tas", &script);
    GTEST_ASSERT_EQ(false, output.m_bError);
}

TEST(Script, LargeMinimal)
{
    srctas::Script script;
    auto output = srctas::Script::ParseFrom("./test_scripts/verylarge_empty.src2tas", &script);
    GTEST_ASSERT_EQ(false, output.m_bError);
}

TEST(Script, BasicParseWorks)
{
    srctas::Script script;
    auto output = srctas::Script::ParseFrom("./test_scripts/basic.src2tas", &script);
    GTEST_ASSERT_EQ(false, output.m_bError);
    GTEST_ASSERT_EQ(script.m_vFrameBulks.size(), 1);
    GTEST_ASSERT_EQ(script.m_vFrameBulks[0].GetFramebulkString(), "s012lj|wasd|jdu12rws|5|2.5|40.5|1|echo \"Hello, world!\"");
}

TEST(Script, MultiLineParseWorks)
{
    srctas::Script script;
    auto output = srctas::Script::ParseFrom("./test_scripts/multi.src2tas", &script);
    GTEST_ASSERT_EQ(false, output.m_bError);
    GTEST_ASSERT_EQ(script.m_vFrameBulks.size(), 2);
    GTEST_ASSERT_EQ(script.m_vFrameBulks[0].GetFramebulkString(), ">>>>>>|>>>>|>>>>>>>>|>|>|>|1|");
    GTEST_ASSERT_EQ(script.m_vFrameBulks[1].GetFramebulkString(), "s012lj|wasd|jdu12rws|5|2.5|40.5|1|echo \"Hello, world!\"");
}

TEST(Script, ManyLines)
{
    srctas::Script script;
    auto output = srctas::Script::ParseFrom("./test_scripts/manylines.src2tas", &script);
    GTEST_ASSERT_EQ(false, output.m_bError);
    GTEST_ASSERT_EQ(script.m_vFrameBulks.size(), 10080);
}

TEST(Script, ManyLinesComplex)
{
    srctas::Script script;
    auto output = srctas::Script::ParseFrom("./test_scripts/manylines_complex.src2tas", &script);
    GTEST_ASSERT_EQ(false, output.m_bError);
    GTEST_ASSERT_EQ(script.m_vFrameBulks.size(), 10081);
}


TEST(Script, MinimalWriteWorks)
{
    const char* filepath = "./temp/test.src2tas";
    srctas::Script script;
    auto output = script.WriteToFile(filepath);
    GTEST_ASSERT_EQ(false, output.m_bError);
    GTEST_ASSERT_EQ(true, script.m_vFrameBulks.empty());

    output = srctas::Script::ParseFrom(filepath, &script);
    GTEST_ASSERT_EQ(false, output.m_bError);
    GTEST_ASSERT_EQ(true, script.m_vFrameBulks.empty());
}
