#include "stdafx.h"
#ifndef OE
#include "..\feature.hpp"
#include "command.hpp"
#include "file.hpp"
#include "interfaces.hpp"
#include "signals.hpp"
#include "spt\srctas\controller.hpp"
#include "spt\sptlib-wrapper.hpp"
#include "string_utils.hpp"
#include "playerio.hpp"
#include "SDK\hl_movedata.h"
#include "tier1\CommandBuffer.h"

typedef bool(__fastcall* _CCommandBuffer__DequeueNextCommand)(CCommandBuffer* thisptr, int edx);
typedef void(__fastcall* _ProcessMovement)(void* thisptr, int edx, void* pPlayer, void* pMove);

ConVar tas_loop_cmd("tas_loop_cmd",
                    "",
                    0,
                    "This command is executed every frame when a TAS is played back or recorded\n");
ConVar tas_log("tas_log", "0", 0, "If enabled, dumps a whole bunch of different stuff into the console.");
ConVar tas_record_optimizebulks("tas_record_optimizebulks",
                                "0",
                                0,
                                "When set to 1, view commands will not be recorded if the view does not change.\n");

// New TASing system
class NewTASFeature : public FeatureWrapper<NewTASFeature>
{
public:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	void OnFrame();
	static void __fastcall HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove);
	static bool __fastcall HOOKED_CCommandBuffer__DequeueNextCommand(CCommandBuffer* thisptr, int edx);

	_ProcessMovement ORIG_ProcessMovement = nullptr;
	_CCommandBuffer__DequeueNextCommand ORIG_CCommandBuffer__DequeueNextCommand = nullptr;
};

struct TASState
{
	srctas::ScriptController controller;
	bool m_bPlayingTAS = false;
	bool m_bRecording = false;
	QAngle m_prevAngle;
	bool m_bAngleValid = false;
};

static TASState tas_state;
static NewTASFeature spt_tas;

bool NewTASFeature::ShouldLoadFeature()
{
	return true;
}

void NewTASFeature::InitHooks()
{
	HOOK_FUNCTION(engine, CCommandBuffer__DequeueNextCommand);

	if (interfaces::gm)
	{
		auto vftable = *reinterpret_cast<void***>(interfaces::gm);
		AddVFTableHook(VFTableHook(vftable, 1, (void*)HOOKED_ProcessMovement, (void**)&ORIG_ProcessMovement),
		               "server");
		InitConcommandBase(tas_log);
	}
}

void NewTASFeature::UnloadFeature() {}

void NewTASFeature::OnFrame()
{
	if (!tas_state.m_bPlayingTAS && !tas_state.m_bRecording)
	{
		return;
	}

	if (tas_state.m_bPlayingTAS)
	{
		srctas::Error error;
		std::string command = tas_state.controller.GetCommandForCurrentTick(error);

		if (error.m_bError)
		{
			DevWarning("Couldn't get command for tick, error: %s\n", error.m_sMessage.c_str());
		}
		else
		{
			EngineConCmd(command.c_str());
		}
	}

	if (tas_state.m_bPlayingTAS && tas_state.controller.LastTick())
	{
		tas_state.m_bPlayingTAS = false;
	}
	else
	{
		tas_state.controller.Advance(1);
	}
}

static char* ToString(const CCommand& command)
{
	static char BUFFER[8192];

	std::size_t arg0_len = strlen(command.Arg(0));
	std::size_t args_len = strlen(command.ArgS());
	memcpy(BUFFER, command.Arg(0), arg0_len);
	BUFFER[arg0_len] = ' ';
	memcpy(BUFFER + arg0_len + 1, command.ArgS(), args_len + 1);

	return BUFFER;
}

static const char* IGNORED_COMMAND_PREFIXES[] = {"exec",
                                                 "sk_",
                                                 "tas_pause",
                                                 "connect",
                                                 "give",
                                                 "autosave",
                                                 "gameui_allowescapetoshow",
                                                 "gameui_hide",
                                                 "changelevel2",
                                                 "cl_predict",
                                                 "dsp_player"};

static bool IsRecordable(const CCommand& command)
{
	if (command.ArgC() == 0)
		return false;

	const char* cmd = command.Arg(0);
	for (auto pref : IGNORED_COMMAND_PREFIXES)
	{
		const char* result = strstr(cmd, pref);
		if (result == cmd)
			return false;
	}

	return true;
}

void __fastcall NewTASFeature::HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove)
{
	CHLMoveData* mv = reinterpret_cast<CHLMoveData*>(pMove);
	QAngle newAngle;

	if (tas_state.m_bRecording)
	{
		bool angleChanged = false;
		newAngle = mv->m_vecAngles;

		if (tas_state.m_bAngleValid)
		{
			angleChanged = (newAngle[0] != tas_state.m_prevAngle[0])
			               || (newAngle[1] != tas_state.m_prevAngle[1])
			               || (newAngle[2] != tas_state.m_prevAngle[2]);
		}

		if (!tas_state.m_bAngleValid || angleChanged || !tas_record_optimizebulks.GetBool())
		{
			tas_state.controller.AddCommands(
			    FormatTempString("_y_spt_setyaw %s", FloatToCString(mv->m_vecAngles[YAW])));
			tas_state.controller.AddCommands(
			    FormatTempString("_y_spt_setpitch %s", FloatToCString(mv->m_vecAngles[PITCH])));
		}

		tas_state.m_bAngleValid = true;
		tas_state.m_prevAngle = mv->m_vecAngles;
	}

	spt_tas.ORIG_ProcessMovement(thisptr, edx, pPlayer, pMove);
}

bool __fastcall NewTASFeature::HOOKED_CCommandBuffer__DequeueNextCommand(CCommandBuffer* thisptr, int edx)
{
	bool rval = spt_tas.ORIG_CCommandBuffer__DequeueNextCommand(thisptr, edx);
	if (rval)
	{
		auto command = thisptr->GetCommand();
		if (tas_state.m_bRecording && IsRecordable(command))
		{
			const char* cmd = ToString(command);
			tas_state.controller.AddCommands(cmd);
		}
	}

	return rval;
}

CON_COMMAND_AUTOCOMPLETEFILE(
    tas_load,
    "Loads and executes an .src2tas script. If an extra ticks argument is given, the script is played back at maximal FPS and without rendering until that many ticks before the end of the script. Usage: tas_load_script [script] [ticks]",
    0,
    "",
    ".src2tas")
{
	if (args.ArgC() == 1)
	{
		Msg("Loads and executes an .src2tas script. Usage: tas_load_script [script]\n");
		return;
	}

	std::string filepath = GetGameDir() + "\\" + args.Arg(1) + ".src2tas";

	auto result = tas_state.controller.LoadFromFile(filepath.c_str());

	if (result.m_bError)
	{
		Warning("%s\n", result.m_sMessage.c_str());
	}
	else
	{
		Msg("Script %s loaded successfully.\n", filepath.c_str());
	}
}

CON_COMMAND(tas_edit_autocollapse, "Automatically collapses multiple framebulks into one when possible")
{
	tas_state.controller.SaveToFile();
}

CON_COMMAND(tas_save, "Saves the TAS.")
{
	tas_state.controller.SaveToFile();
}

CON_COMMAND(tas_init, "Inits an empty TAS.")
{
	if (args.ArgC() == 1)
	{
		Msg("Inits an empty .src2tas script. Usage: tas_init [script]\n");
		return;
	}

	std::string filepath = GetGameDir() + "\\" + args.Arg(1) + ".src2tas";
	tas_state.controller.InitEmptyScript(filepath.c_str());
}

CON_COMMAND(tas_play, "Starts playing the TAS.")
{
	tas_state.controller.SetToTick(0);
	tas_state.m_bPlayingTAS = true;
}

CON_COMMAND(tas_skip, "Skip to tick in TAS.")
{
	tas_state.controller.SetToTick(0);
	tas_state.m_bPlayingTAS = false;
	tas_state.m_bRecording = false;
}

CON_COMMAND(tas_record_start, "Starts recording a TAS.")
{
	tas_state.m_bPlayingTAS = false;
	tas_state.m_bRecording = true;
	tas_state.m_bAngleValid = false;
}

CON_COMMAND(tas_record_stop, "Stops a TAS recording.")
{
	tas_state.m_bRecording = false;
}

CON_COMMAND(tas_resume, "Resumes playing the TAS.")
{
	tas_state.m_bPlayingTAS = true;
}

CON_COMMAND(tas_pause, "Pauses TAS playback.")
{
	tas_state.m_bPlayingTAS = false;
	tas_state.m_bRecording = false;
}

void NewTASFeature::LoadFeature()
{
	if (FrameSignal.Works)
	{
		FrameSignal.Connect(this, &NewTASFeature::OnFrame);
		InitCommand(tas_init);
		InitCommand(tas_load);
		InitCommand(tas_pause);
		InitCommand(tas_play);
		InitCommand(tas_record_start);
		InitCommand(tas_record_stop);
		InitCommand(tas_resume);
		InitCommand(tas_save);
		InitCommand(tas_skip);
		InitConcommandBase(tas_loop_cmd);
		InitConcommandBase(tas_record_optimizebulks);
	}
}
#else
#include "convar.hpp"
ConVar tas_log("tas_log", "0", 0, "If enabled, dumps a whole bunch of different stuff into the console.");
#endif