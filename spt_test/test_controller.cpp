#include "gtest/gtest.h"
#include "spt/srctas/controller.hpp"

TEST(Controller, AddFrameBulkWorks)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.InitEmptyScript("/tmp/test.src2tas");
    controller.Record_Start();
    GTEST_ASSERT_EQ(error.m_bError, false);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
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
    controller.Record_Start();
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.TEST_Advance(10);
    std::cout << error.m_sMessage << std::endl;
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
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
    controller.Record_Start();
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.TEST_Advance(10);
    auto state = controller.GetCurrentFramebulk();
    GTEST_ASSERT_NE(state.m_sCurrent, nullptr);
    GTEST_ASSERT_EQ(state.m_iTickInBulk, 10);
}

TEST(Controller, FrameBulkAdvanceWorksNegative)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.InitEmptyScript("/tmp/test.src2tas");
    controller.Record_Start();
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.TEST_Advance(10);
    error = controller.TEST_Advance(-5);
    auto state = controller.GetCurrentFramebulk();
    GTEST_ASSERT_NE(state.m_sCurrent, nullptr);
    GTEST_ASSERT_EQ(state.m_iTickInBulk, 5);
}


TEST(Controller, FrameBulkSplittingWorks2)
{
    srctas::ScriptController controller;
    srctas::Error error;
    error = controller.InitEmptyScript("/tmp/test.src2tas");
    controller.Record_Start();
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.TEST_Advance(10);
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.TEST_Advance(-5);
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+attack2");

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
    controller.Record_Start();
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.TEST_Advance(10);
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.TEST_Advance(-1);
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+attack2");

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
    controller.Record_Start();
    controller.TEST_Advance(10);
    controller.OnCommandExecuted("+tas_strafe;+attack");
    controller.TEST_Advance(-1);
    controller.OnCommandExecuted("+attack2");
    controller.TEST_Advance(1);
    controller.OnCommandExecuted("tas_strafe_type 0; tas_strafe_jumptype 1; +tas_lgagst");

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
    controller.Record_Start();
    controller.TEST_Advance(100);
    controller.OnCommandExecuted("+forward");
    controller.TEST_Advance(200);
    controller.OnCommandExecuted("-forward; echo stop");

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
    controller.Record_Start();
    controller.TEST_Advance(10);
    controller.OnCommandExecuted("+tas_strafe;+attack");
    controller.TEST_Advance(-1);
    controller.OnCommandExecuted("+attack2");
    controller.TEST_Advance(1);
    controller.OnCommandExecuted("tas_strafe_type 0; tas_strafe_jumptype 1; +tas_lgagst");

    std::string output1 = controller.GetFrameBulkHistory(3, error);
    std::string expectedHistory =   ">>>>>>|>>>>|>>>>>>>>|>|>|>|9|\n"
                                    ">>>>>>|>>>>|>>>>2>>>|>|>|>|1|\n"
                                    "s01>l>|>>>>|>>>1>>>>|>|>|>|1|";

    GTEST_ASSERT_EQ(output1, expectedHistory);

    controller.TEST_Advance(-1);
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
    GTEST_ASSERT_EQ(controller.IsRecording(), true);
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);
    controller.OnFrame();
    controller.OnCommandExecuted("echo test");
    controller.Record_Stop();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
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


TEST(Controller, RewindWorks)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/basic.src2tas");
    std::string executed;

    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);

    controller.SetRewindState(1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);

    controller.SetRewindState(-1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);
}


TEST(Controller, CameraWorks)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/basic.src2tas");
    std::string executed;
    int rewind = 0;
    float pos1[3] = {0, 0, 0};
    float ang1[3] = {0, 0, 0};
    float pos2[3] = {1, 2, 3};
    float ang2[3] = {4, 5, 6};

    float pos[3] = {-999, -999, -999};
    float ang[3] = {-999, -999, -999};

    controller.m_fResetView = [&pos, &ang]()
    {
        pos[0] = -1;
        pos[1] = -1;
        pos[2] = -1;
        ang[0] = -1;
        ang[1] = -1;
        ang[2] = -1;
    };

    controller.m_fSetView = [&pos, &ang](auto newpos, auto newang)
    {
        pos[0] = newpos[0];
        pos[1] = newpos[1];
        pos[2] = newpos[2];
        ang[0] = newang[0];
        ang[1] = newang[1];
        ang[2] = newang[2];
    };

    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);

    controller.SetRewindState(1);
    controller.OnMove(pos1, ang1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(pos[0], -1); // Should reset view
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);

    controller.SetRewindState(1);
    controller.OnMove(pos2, ang2);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 2);

    controller.SetRewindState(-1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    GTEST_ASSERT_EQ(pos[0], 1);

    controller.SetRewindState(-1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);
    GTEST_ASSERT_EQ(pos[0], 0);

    // Extra rewind should do nothing, already at the beginning
    controller.SetRewindState(-1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);
    GTEST_ASSERT_EQ(pos[0], 0);

    // Rewind forwards again
    controller.SetRewindState(1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    GTEST_ASSERT_EQ(pos[0], 1);

    controller.SetRewindState(1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 2);
    GTEST_ASSERT_EQ(pos[0], -1);
}

TEST(Controller, DoesNotAutoPlay)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);
}

TEST(Controller, SkipWorks)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Skip(1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
}

TEST(Controller, SkipWorks2)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Skip(2);
    controller.OnFrame();
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 2);
    controller.OnFrame(); // Extra tick here does nothing
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 2);

    controller.Skip(1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
}

TEST(Controller, CanOverridePause)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Skip(2);
    controller.OnFrame();
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 2);
    controller.SetPaused(false);
    controller.OnFrame(); // Extra tick here does something, we have set paused to false
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 3);
}


TEST(Controller, SkipDoesNotStartFromBeginningWhenSkipPointAheadOfPlayback)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Skip(1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    controller.Skip(2);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 2);
}

TEST(Controller, SkipJumpsToPlaybackTick)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Skip(5);
    for(int i=0; i < 5; ++i)
        controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 5);
    controller.TEST_Advance(-4);
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    controller.Skip(6);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 6);
    GTEST_ASSERT_EQ(controller.m_iCurrentPlaybackTick, 6);
}

TEST(Controller, PlaybackAutoRollsback)
{
    srctas::ScriptController controller;
    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Skip(5);
    for(int i=0; i < 5; ++i)
        controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 5);
    for(int i=0; i < 4; ++i)
    {
        controller.SetRewindState(-1);
        controller.OnFrame();
    }
    controller.OnCommandExecuted("echo test");
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    controller.SetRewindState(1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    GTEST_ASSERT_EQ(controller.m_iCurrentPlaybackTick, 1);
}


TEST(Controller, PlaybackDoesNotRollbackBad)
{
    srctas::ScriptController controller;
    int rewind = 0;

    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Skip(5);
    for(int i=0; i < 5; ++i)
        controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 5);
    controller.SetRewindState(-1);
    controller.OnFrame();
    controller.OnCommandExecuted("echo test");
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 4);
    controller.SetRewindState(-1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 3);
}

TEST(Controller, SkipTimescaleWorks)
{
    srctas::ScriptController controller;
    float timescale = 1;
    controller.m_fSetTimeScale = [&timescale](float t) 
    {
        timescale = t;
    };

    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Skip(5);
    for(int i=0; i < 5; ++i)
    {
        GTEST_ASSERT_GT(timescale, 1);
        controller.OnFrame();
    }
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 5);
    GTEST_ASSERT_EQ(timescale, 1);
}

TEST(Controller, PlayDoesntTimescale)
{
    srctas::ScriptController controller;
    float timescale = 1;
    controller.m_fSetTimeScale = [&timescale](float t) 
    {
        timescale = t;
    };

    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Play();
    for(int i=0; i < 2; ++i)
    {
        GTEST_ASSERT_EQ(timescale, 1);
        controller.OnFrame();
    }
}

TEST(Controller, PauseWorks)
{
    srctas::ScriptController controller;
    float timescale = 1;
    controller.m_fSetTimeScale = [&timescale](float t) 
    {
        timescale = t;
    };

    auto error = controller.LoadFromFile("./test_scripts/multi.src2tas");
    controller.Play();
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    controller.Pause();
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);
    controller.Pause();
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 2);
}

TEST(Controller, RecordDoesntMultiplyCommands)
{
    srctas::ScriptController controller;
    std::string output;
    controller.m_fExecConCmd = [&output](const char* cmd) { output = cmd; };

    auto error = controller.InitEmptyScript("test");
    controller.Record_Start();
    controller.OnCommandExecuted("map c1a0");
    controller.OnFrame();
    GTEST_ASSERT_EQ(output, "");
}

TEST(Controller, PausedRecordsCommands)
{
    srctas::ScriptController controller;
    std::string output;
    controller.m_fExecConCmd = [&output](const char* cmd) { output = cmd; };

    auto error = controller.InitEmptyScript("test");
    controller.m_bAutoPause = false;
    controller.Play();
    controller.Pause();
    controller.SetPaused(true);
    controller.OnFrame();
    controller.OnCommandExecuted("map c1a0");
    controller.OnFrame();
    GTEST_ASSERT_EQ(output, "");
    controller.SetPaused(false);
    controller.OnFrame();
    GTEST_ASSERT_EQ(output.empty(), false);
}


TEST(Controller, PauseAbductsCommands)
{
    srctas::ScriptController controller;
    std::string output;
    controller.m_fExecConCmd = [&output](const char* cmd) { output = cmd; };

    auto error = controller.InitEmptyScript("test");
    controller.m_bAutoPause = false;
    GTEST_ASSERT_EQ(controller.ShouldAbductCommand(), false);
    controller.Play();
    controller.Pause();
    controller.SetPaused(true);
    GTEST_ASSERT_EQ(controller.ShouldAbductCommand(), true);
    controller.SetPaused(false);
    GTEST_ASSERT_EQ(controller.ShouldAbductCommand(), false);
}

TEST(Controller, RecordingPlayback)
{
    srctas::ScriptController controller;
    std::string commands;
    int count = 0;

    controller.m_fExecConCmd = [&count, &commands](auto cmd) {
        commands = cmd;
        ++count;
    };

    auto error = controller.LoadFromFile("./test_scripts/recording.src2tas");
    controller.OnCommandExecuted("tas_play");
    controller.Play();
    controller.OnFrame();
    GTEST_ASSERT_EQ(count, 1);
    GTEST_ASSERT_EQ(commands, ";map testchmb_a_06;host_framerate 0.015;y_spt_autojump 1;fps_max 66.6666;sv_cheats 1");
    commands.clear();
    controller.OnFrame();
    GTEST_ASSERT_EQ(count, 1);
    GTEST_ASSERT_EQ(commands, "");
}