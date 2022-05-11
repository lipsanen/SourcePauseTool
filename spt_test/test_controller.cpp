#include "gtest/gtest.h"
#include "spt/srctas/controller.hpp"
#include <algorithm>

class ControllerTest : public ::testing::Test {

    void SetUp() override {
        controller.m_bAutoPause = true;
        controller.InitEmptyScript("/tmp/test.src2tas");
    }

public:
    srctas::Error TEST_Skip(int ticks)
    {
        controller.Skip(ticks);

        int previous;
        do
        {
            previous = controller.m_iCurrentTick;
            srctas::Error err;
            int totalTicks = controller.GetTotalTicks(err);
            controller.OnFrame();
        } while(controller.m_iCurrentTick != previous);

        return srctas::Error();
    }

    srctas::Error TEST_Advance(int ticks)
    {
        int state = ticks > 0 ? 1 : -1;
        bool autopauseBefore = controller.m_bAutoPause;
        controller.m_bAutoPause = false;

        if(state > 0)
        {
            controller.m_bPaused = false;
        }
        else
        {
            controller.m_bPaused = true;
        }

        for(int i=0; i < std::abs(ticks); ++i)
        {
            controller.SetRewindState(state);
            auto error = controller.OnFrame();

            if(error.m_bError)
            {
                return error;
            }
        }

        controller.m_bAutoPause = autopauseBefore;
        return srctas::Error();
    }

    srctas::ScriptController controller;
};

TEST_F(ControllerTest, AddFrameBulkWorks)
{
    srctas::Error error;
    controller.Record_Start();
    GTEST_ASSERT_EQ(error.m_bError, false);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
    controller.OnFrame();
    TEST_Advance(-1);
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
    for(int i=0; i < 10; ++i)
        controller.OnFrame();
    std::cout << error.m_sMessage << std::endl;
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
    controller.OnFrame();
    GTEST_ASSERT_EQ(false, error.m_bError);

    std::string output1 = controller.m_sScript.m_vFrameBulks[0].GetFramebulkString();
    std::string output2 = controller.m_sScript.m_vFrameBulks[1].GetFramebulkString();
    GTEST_ASSERT_EQ(output1, ">>>>>>|>>>>|>>>>>>>>|>|>|>|10|");
    GTEST_ASSERT_EQ(output2, "s>>>>>|>>>>|>>>1>>>>|>|>|>|1|");
}

TEST_F(ControllerTest, FrameBulkAdvanceWorks)
{
    srctas::Error error;
    controller.Record_Start();
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = TEST_Advance(10);
    auto state = controller.GetCurrentFramebulk();
    GTEST_ASSERT_NE(state.m_sCurrent, nullptr);
    GTEST_ASSERT_EQ(state.m_iTickInBulk, 10);
}

TEST_F(ControllerTest, FrameBulkAdvanceWorksNegative)
{
    auto error = controller.Record_Start();
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = controller.OnCommandExecuted("+tas_strafe;+attack");
    GTEST_ASSERT_EQ(false, error.m_bError);
    error = TEST_Advance(10);
    error = TEST_Advance(-5);
    auto state = controller.GetCurrentFramebulk();
    GTEST_ASSERT_NE(state.m_sCurrent, nullptr);
    GTEST_ASSERT_EQ(state.m_iTickInBulk, 5);
}

TEST_F(ControllerTest, FrameBulkHistoryWorks)
{
    srctas::Error error;
    controller.Record_Start();
    TEST_Advance(9);
    controller.OnCommandExecuted("+attack2");
    TEST_Advance(1);
    controller.OnCommandExecuted("+tas_strafe;+attack;tas_strafe_type 0; tas_strafe_jumptype 1; +tas_lgagst");
    TEST_Advance(1);

    std::string output1 = controller.GetFrameBulkHistory(3, error);
    std::string expectedHistory =   ">>>>>>|>>>>|>>>>>>>>|>|>|>|9|\n"
                                    ">>>>>>|>>>>|>>>>2>>>|>|>|>|1|\n"
                                    "s01>l>|>>>>|>>>1>>>>|>|>|>|1|";

    GTEST_ASSERT_EQ(output1, expectedHistory);

    TEST_Advance(-2);
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

TEST_F(ControllerTest, RecordWorks)
{
    controller.Record_Start();
    GTEST_ASSERT_EQ(controller.IsRecording(), true);
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);
    controller.OnFrame();
    controller.OnCommandExecuted("echo test");
    controller.OnFrame();
    controller.Record_Stop();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 2);
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

TEST_F(ControllerTest, RecordingPlayback)
{
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

TEST_F(ControllerTest, CommandsGetBufferedDuringPause)
{
    controller.m_bAutoPause = false;
    controller.m_bPaused = false;
    controller.InitEmptyScript("test.src2tas");
    controller.Skip(1);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 1);

    controller.m_bPaused = true;
    controller.OnCommandExecuted("echo test");
    controller.OnFrame(); // Paused frame

    controller.m_bPaused = false;
    controller.Record_Start();
    controller.OnFrame();
    controller.Record_Stop();
    controller.m_bPaused = true;
    controller.OnFrame();

    GTEST_ASSERT_EQ(controller.m_sScript.m_vFrameBulks.size(), 2);
    auto bulk = controller.m_sScript.m_vFrameBulks[1];
    GTEST_ASSERT_EQ(bulk.GetCommand(), ";echo test");
}

TEST_F(ControllerTest, RecordAssertsLastTick)
{
    controller.LoadFromFile("./test_scripts/recording.src2tas");
    TEST_Skip(-2);
    auto error = controller.Record_Start();
    GTEST_ASSERT_EQ(error.m_bError, true);
    TEST_Skip(-1);
    error = controller.Record_Start();
    GTEST_ASSERT_EQ(error.m_bError, false);
}

TEST_F(ControllerTest, SkipNegativeWorks)
{
    controller.LoadFromFile("./test_scripts/recording.src2tas");
    TEST_Skip(-2);
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 284);
    TEST_Skip(-1);
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 285);
}


TEST_F(ControllerTest, PausedRecordingWorks)
{
    TEST_Skip(5);
    controller.m_bAutoPause = false;
    controller.m_bPaused = true;
    controller.Record_Start();
    controller.OnFrame();
    controller.OnCommandExecuted("echo test");
    controller.OnFrame();
    controller.Record_Stop();
    TEST_Advance(1);
    auto framebulk = controller.m_sScript.m_vFrameBulks[1];
    GTEST_ASSERT_EQ(framebulk.GetCommand(), ";echo test");
}


TEST_F(ControllerTest, CantRewindToNegative)
{
    TEST_Skip(0);
    controller.SetRewindState(-1);
    controller.m_bPaused = true;
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_iCurrentTick, 0);
}

TEST_F(ControllerTest, AnglesGetSaved)
{
    float pos[3];
    float ang[3];
    pos[0] = pos[1] = pos[2] = 1.0f;
    ang[0] = ang[1] = ang[2] = 2.0f;
    controller.Record_Start();
    controller.OnMove(pos, ang);
    controller.m_bPaused = false;
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_sScript.m_vFrameBulks.size(), 1);
    auto& framebulk = controller.m_sScript.m_vFrameBulks[0];
    GTEST_ASSERT_EQ(framebulk.GetCommand(), ";_y_spt_setpitch 2;_y_spt_setyaw 2");
}

TEST_F(ControllerTest, AngleChangeDetectionWorks)
{
    float pos[3];
    float ang[3];
    pos[0] = pos[1] = pos[2] = 1.0f;
    ang[0] = ang[1] = ang[2] = 2.0f;
    controller.Record_Start();
    controller.OnMove(pos, ang);
    controller.OnFrame();
    controller.OnMove(pos, ang);
    controller.OnFrame();
    ang[0] = ang[1] = ang[2] = 3.0f;
    controller.OnMove(pos, ang);
    controller.OnFrame();
    GTEST_ASSERT_EQ(controller.m_sScript.m_vFrameBulks.size(), 2);
    auto& framebulk1 = controller.m_sScript.m_vFrameBulks[0];
    GTEST_ASSERT_EQ(framebulk1.m_iTicks, 2);
    GTEST_ASSERT_EQ(framebulk1.GetCommand(), ";_y_spt_setpitch 2;_y_spt_setyaw 2");
    auto& framebulk2 = controller.m_sScript.m_vFrameBulks[1];
    GTEST_ASSERT_EQ(framebulk2.GetCommand(), ";_y_spt_setpitch 3;_y_spt_setyaw 3");
}

TEST_F(ControllerTest, StopDoesntHang)
{
    float pos[3];
    float ang[3];
    pos[0] = pos[1] = pos[2] = 1.0f;
    ang[0] = ang[1] = ang[2] = 2.0f;
    controller.Record_Start();
    controller.OnMove(pos, ang);
    controller.OnFrame();
    controller.OnMove(pos, ang);
    controller.OnFrame();
    controller.OnFrame();
    controller.Record_Stop();
    controller.OnFrame();
    controller.SetRewindState(-1);
    controller.OnFrame();
    EXPECT_EQ(controller.m_iCurrentTick, 3);
    controller.Play();
    controller.OnFrame();
    EXPECT_EQ(controller.m_iCurrentTick, 1);
}

TEST_F(ControllerTest, Record_StartCorrectState)
{
    controller.Record_Start();
    EXPECT_GE(controller.GetPlayState(), 0);
}

TEST_F(ControllerTest, Record_CurrentMatchesPlayback)
{
    controller.Record_Start();
    controller.OnFrame();
    controller.Record_Stop();
    controller.OnFrame();
    EXPECT_EQ(controller.m_iCurrentPlaybackTick, controller.m_iCurrentTick);
    controller.Record_Start();
    controller.OnFrame();
    EXPECT_EQ(controller.m_iCurrentPlaybackTick, controller.m_iCurrentTick);
    controller.OnFrame();
    EXPECT_EQ(controller.m_iCurrentPlaybackTick, controller.m_iCurrentTick);
}
