#include "gtest/gtest.h"
#include "spt\srctas\controller.hpp"

TEST(Controller, AddFrameBulkWorks)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.InitEmptyScript("/tmp/test.src2tas");
    GTEST_ASSERT_EQ(error.m_bError, false);
    error = controller.AddCommands("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(error.m_bError, false);
    
    auto commands = controller.GetCommandForCurrentTick(error);
    GTEST_ASSERT_EQ(error.m_bError, false);
    GTEST_ASSERT_EQ(commands, ";+tas_strafe;+attack");
    std::string input = "s>>>>>|>>>>|>>>1>>>>|>|>|>|1|";
    auto state = controller.GetCurrentFramebulk();
    GTEST_ASSERT_NE(state.m_sCurrent, nullptr);
    GTEST_ASSERT_EQ(state.m_sCurrent->GetFramebulkString(), input);
}

TEST(Controller, FrameBulkSplittingWorks)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.InitEmptyScript("/tmp/test.src2tas");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.Advance(10);
    std::cout << error.m_sMessage << std::endl;
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.AddCommands("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);

    std::string output1 = controller.m_sScript.m_vFrameBulks[0].GetFramebulkString();
    std::string output2 = controller.m_sScript.m_vFrameBulks[1].GetFramebulkString();
    GTEST_ASSERT_EQ(output1, ">>>>>>|>>>>|>>>>>>>>|>|>|>|10|");
    GTEST_ASSERT_EQ(output2, "s>>>>>|>>>>|>>>1>>>>|>|>|>|1|");
}

TEST(Controller, FrameBulkAdvanceWorks)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.InitEmptyScript("/tmp/test.src2tas");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.AddCommands("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.Advance(10);
    auto state = controller.GetCurrentFramebulk();
    GTEST_ASSERT_NE(state.m_sCurrent, nullptr);
    GTEST_ASSERT_EQ(state.m_iTickInBulk, 10);
}

TEST(Controller, FrameBulkAdvanceWorksNegative)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.InitEmptyScript("/tmp/test.src2tas");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.AddCommands("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.Advance(10);
    error = controller.Advance(-5);
    auto state = controller.GetCurrentFramebulk();
    GTEST_ASSERT_NE(state.m_sCurrent, nullptr);
    GTEST_ASSERT_EQ(state.m_iTickInBulk, 5);
}


TEST(Controller, FrameBulkSplittingWorks2)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.InitEmptyScript("/tmp/test.src2tas");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.Advance(10);
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.AddCommands("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.Advance(-5);
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.AddCommands("+attack2");

    std::string output1 = controller.m_sScript.m_vFrameBulks[0].GetFramebulkString();
    std::string output2 = controller.m_sScript.m_vFrameBulks[1].GetFramebulkString();
    std::string output3 = controller.m_sScript.m_vFrameBulks[2].GetFramebulkString();
    GTEST_ASSERT_EQ(output1, ">>>>>>|>>>>|>>>>>>>>|>|>|>|5|");
    GTEST_ASSERT_EQ(output2, ">>>>>>|>>>>|>>>>2>>>|>|>|>|5|");
    GTEST_ASSERT_EQ(output3, "s>>>>>|>>>>|>>>1>>>>|>|>|>|1|");
}

TEST(Controller, FrameBulkSplittingWorks3)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.InitEmptyScript("/tmp/test.src2tas");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.Advance(10);
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.AddCommands("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.Advance(-1);
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.AddCommands("+attack2");

    std::string output1 = controller.m_sScript.m_vFrameBulks[0].GetFramebulkString();
    std::string output2 = controller.m_sScript.m_vFrameBulks[1].GetFramebulkString();
    std::string output3 = controller.m_sScript.m_vFrameBulks[2].GetFramebulkString();
    GTEST_ASSERT_EQ(output1, ">>>>>>|>>>>|>>>>>>>>|>|>|>|9|");
    GTEST_ASSERT_EQ(output2, ">>>>>>|>>>>|>>>>2>>>|>|>|>|1|");
    GTEST_ASSERT_EQ(output3, "s>>>>>|>>>>|>>>1>>>>|>|>|>|1|");
}

TEST(Controller, FrameBulkSplittingWorks4)
{
    srctas::ScriptController controller;
    controller.InitEmptyScript("/tmp/test.src2tas");
    controller.Advance(10);
    controller.AddCommands("+tas_strafe;+attack");
    controller.Advance(-1);
    controller.AddCommands("+attack2");
    controller.Advance(1);
    controller.AddCommands("tas_strafe_type 0; tas_strafe_jumptype 1; +tas_lgagst");

    std::string output1 = controller.m_sScript.m_vFrameBulks[0].GetFramebulkString();
    std::string output2 = controller.m_sScript.m_vFrameBulks[1].GetFramebulkString();
    std::string output3 = controller.m_sScript.m_vFrameBulks[2].GetFramebulkString();
    GTEST_ASSERT_EQ(output1, ">>>>>>|>>>>|>>>>>>>>|>|>|>|9|");
    GTEST_ASSERT_EQ(output2, ">>>>>>|>>>>|>>>>2>>>|>|>|>|1|");
    GTEST_ASSERT_EQ(output3, "s01>l>|>>>>|>>>1>>>>|>|>|>|1|");
}

TEST(Controller, FrameBulkSplittingWorks5)
{
    srctas::ScriptController controller;
    controller.InitEmptyScript("/tmp/test.src2tas");
    controller.Advance(100);
    controller.AddCommands("+forward");
    controller.Advance(200);
    controller.AddCommands("-forward; echo stop");

    std::string output1 = controller.m_sScript.m_vFrameBulks[0].GetFramebulkString();
    std::string output2 = controller.m_sScript.m_vFrameBulks[1].GetFramebulkString();
    //std::string output3 = controller.m_sScript.m_vFrameBulks[2].GetFramebulkString();
    GTEST_ASSERT_EQ(output1, ">>>>>>|>>>>|>>>>>>>>|>|>|>|100|");
    GTEST_ASSERT_EQ(output2, ">>>>>>|w>>>|>>>>>>>>|>|>|>|200|");
    //GTEST_ASSERT_EQ(output3, ">>>>>>|->>>|>>>>>>>>|>|>|>|1|echo stop");
}


TEST(Controller, FrameBulkHistoryWorks)
{
    srctas::ScriptController controller;
    srctas::Error error;
    controller.InitEmptyScript("/tmp/test.src2tas");
    controller.Advance(10);
    controller.AddCommands("+tas_strafe;+attack");
    controller.Advance(-1);
    controller.AddCommands("+attack2");
    controller.Advance(1);
    controller.AddCommands("tas_strafe_type 0; tas_strafe_jumptype 1; +tas_lgagst");

    std::string output1 = controller.GetFrameBulkHistory(3, error);
    std::string expectedHistory =   ">>>>>>|>>>>|>>>>>>>>|>|>|>|9|\n"
                                    ">>>>>>|>>>>|>>>>2>>>|>|>|>|1|\n"
                                    "s01>l>|>>>>|>>>1>>>>|>|>|>|1|";

    GTEST_ASSERT_EQ(output1, expectedHistory);

    controller.Advance(-1);
    std::string output2 = controller.GetFrameBulkHistory(3, error);
    std::string expectedHistory2 =  ">>>>>>|>>>>|>>>>>>>>|>|>|>|9|\n"
                                    ">>>>>>|>>>>|>>>>2>>>|>|>|>|1|";

    GTEST_ASSERT_EQ(output2, expectedHistory2);
}

TEST(Controller, LoadWorks)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.LoadFromFile("./test_scripts/basic.src2tas");
    GTEST_ASSERT_EQ(false, error.m_bError);
    auto output = controller.m_sScript.m_vFrameBulks[0].GetFramebulkString();
    GTEST_ASSERT_EQ("s012lj|wasd|jdu12rws|5|2.5|40.5|1|echo \"Hello, world!\"", output);
}

TEST(Controller, LoadInvalidFails)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.LoadFromFile("./noscripthere.src2tas");
    GTEST_ASSERT_EQ(true, error.m_bError);
    std::cout << error.m_sMessage << std::endl;
}

TEST(Controller, NoFileSaveFails)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.SaveToFile();
    GTEST_ASSERT_EQ(true, error.m_bError);
    std::cout << error.m_sMessage << std::endl;
}

TEST(Controller, RenameWorks)
{   
    srctas::ScriptController controller;
    srctas::Error error;
    const char* newPath = "./temp/basic.src2tas";
    error = controller.LoadFromFile("./test_scripts/basic.src2tas");
    unlink(newPath);
    
    struct stat s;
    GTEST_ASSERT_NE(stat(newPath, &s), 0);
    controller.SaveToFile(newPath);
    GTEST_ASSERT_EQ(stat(newPath, &s), 0);
}

TEST(Controller, RecordWorks)
{
    srctas::ScriptController controller;

    controller.InitEmptyScript("test.src2tas");
    controller.Record_Start();
    GTEST_ASSERT_EQ(controller.m_bRecording, true);
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);
    controller.OnFrame();
    controller.AddCommands("echo test");
    controller.Record_Stop();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    GTEST_ASSERT_EQ(controller.m_bRecording, false);
    srctas::Error err;
    std::string history = controller.GetFrameBulkHistory(2, err);
    std::string expected = ">>>>>>|>>>>|>>>>>>>>|>|>|>|1|\n"
                           ">>>>>>|>>>>|>>>>>>>>|>|>|>|1|echo test";
    GTEST_ASSERT_EQ(history, expected);
}

TEST(Controller, OnFrameExecutesCommands)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/basic.src2tas");
    std::string executed;
    int timesExecuted = 0;
    controller.m_fExecConCmd = [&](auto cmd) 
    {
        executed = cmd;
        ++timesExecuted;
    };

    controller.Play();
    controller.OnFrame();
    std::cout << executed << std::endl;
    GTEST_ASSERT_EQ(timesExecuted, 1);
    GTEST_ASSERT_NE(executed, "");
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);

    // Extra OnFrame calls should do nothing, we're at the end of script
    controller.OnFrame();
    GTEST_ASSERT_NE(executed, "");
    GTEST_ASSERT_EQ(timesExecuted, 1);
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
}