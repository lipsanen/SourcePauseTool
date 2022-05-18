#include "stdafx.h"
#include "..\feature.hpp"
#include "command.hpp"
#include "hud.hpp"
#include "file.hpp"
#include "generic.hpp"
#include "interfaces.hpp"
#include "math.hpp"
#include "signals.hpp"
#include "spt\srctas\controller.hpp"
#include "spt\sptlib-wrapper.hpp"
#include "string_utils.hpp"
#include "pause.hpp"
#include "playerio.hpp"
#include "taslogging.hpp"
#include "SDK\hl_movedata.h"
#include "tier1\CommandBuffer.h"
#include "view_shared.h"

typedef bool(__fastcall* _CCommandBuffer__DequeueNextCommand)(CCommandBuffer* thisptr, int edx);
typedef void(__fastcall* _ProcessMovement)(void* thisptr, int edx, void* pPlayer, void* pMove);

ConVar tas_loop_cmd("tas_loop_cmd",
                    "",
                    0,
                    "This command is executed every frame when a TAS is played back or recorded\n");
ConVar tas_record_optimizebulks("tas_record_optimizebulks",
                                "0",
                                0,
                                "When set to 1, view commands will not be recorded if the view does not change.\n");
CON_COMMAND_TOGGLE(tas_forward, "Play back TAS forward");
CON_COMMAND_TOGGLE(tas_backward, "Play back TAS backward");
ConVar tas_hud("tas_hud", "1", 0, "Enables TAS hud");

// New TASing system
class NewTASFeature : public FeatureWrapper<NewTASFeature>
{
public:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	void OnFrame();
	uintptr_t ORIG__Host_RunFrame = 0;
	CDECL_DETOUR(void, Host_AccumulateTime, float dt);
	static void __fastcall HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove);
	static bool __fastcall HOOKED_CCommandBuffer__DequeueNextCommand(CCommandBuffer* thisptr, int edx);

	_ProcessMovement ORIG_ProcessMovement = nullptr;
	_CCommandBuffer__DequeueNextCommand ORIG_CCommandBuffer__DequeueNextCommand = nullptr;
	float* pHost_Frametime = nullptr;
	float* pHost_Realtime = nullptr;
};

struct TASState
{
	srctas::ScriptController controller;
	bool m_bSetView = false;
	float m_currentAngle[3];
	float m_currentPos[3];

	TASState()
	{
		controller.m_bAutoPause = false;
	}

	void SetView(float* pos, float* ang)
	{
		m_bSetView = true;
		for (int i = 0; i < 3; ++i)
		{
			m_currentPos[i] = pos[i];
			m_currentAngle[i] = ang[i];
		}
	}

	void ResetView()
	{
		m_bSetView = false;
	}

	void SetupView(void* cameraView, int& nClearFlags, int& whatToDraw)
	{
		if(!m_bSetView)
			return;

		m_bSetView = false;
		CViewSetup* view = reinterpret_cast<CViewSetup*>(cameraView);
		for (int i = 0; i < 3; ++i)
		{
			view->origin[i] = m_currentPos[i];
			view->angles[i] = m_currentAngle[i];
		}
	}
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
	FIND_PATTERN(engine, _Host_RunFrame);
	HOOK_FUNCTION(engine, Host_AccumulateTime);

	if (interfaces::gm)
	{
		auto vftable = *reinterpret_cast<void***>(interfaces::gm);
		AddVFTableHook(VFTableHook(vftable, 1, (void*)HOOKED_ProcessMovement, (void**)&ORIG_ProcessMovement),
		               "server");
	}
}

void NewTASFeature::UnloadFeature() {}

int GetRewindState()
{
	float state = keyState_tas_forward.KeyState() - keyState_tas_backward.KeyState();

	if (state > 0)
		return 1;
	else if (state == 0)
		return 0;
	else
		return -1;
}

void NewTASFeature::OnFrame()
{
	srctas::Error error;
	error = tas_state.controller.OnFrame();
	tas_state.controller.SetRewindState(GetRewindState());

	if(error.m_bError)
	{
		DevWarning(error.m_sMessage.c_str());
		return;
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
                                                 "changelevel2",
                                                 "cl_predict",
                                                 "dsp_player",
												 "+tas_forward",
												 "-tas_forward",
												 "+tas_backward",
												 "-tas_backward",
												 "tas_record"};

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

	// Make sure the command is not an alias
	return interfaces::g_pCVar->FindCommand(cmd) != nullptr;
}

CDECL_HOOK(void, NewTASFeature, Host_AccumulateTime, float dt)
{
	if(tas_state.controller.ShouldPause() && !spt_pause.InLoad() && spt_tas.ORIG__Host_RunFrame)
	{
		*spt_tas.pHost_Realtime += dt;
		*spt_tas.pHost_Frametime = 0;
		tas_state.controller.SetPaused(true);
	}
	else
	{
		tas_state.controller.SetPaused(false);
		spt_tas.ORIG_Host_AccumulateTime(dt);
	}
}

void __fastcall NewTASFeature::HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove)
{
	CHLMoveData* mv = reinterpret_cast<CHLMoveData*>(pMove);
	QAngle newAngle;
	float ang[3];
	float pos[3];
	utils::VectorCopy(pos, spt_generic.GetCameraOrigin());
	EngineGetViewAngles(ang);
	tas_state.controller.OnMove(pos, ang);

	spt_taslogging.Log(
	    "PRE ProcessMovement: POS (%.8f, %.8f, %.8f), ANG (%.8f, %.8f, %.8f), PUNCH (%.8f, %.8f, %.8f), VEL (%.8f, %.8f, %.8f), MOVE (%.8f, %.8f, %.8f), MAX %.8f",
	    mv->GetAbsOrigin().x,
	    mv->GetAbsOrigin().y,
	    mv->GetAbsOrigin().z,
	    mv->m_vecViewAngles.x,
	    mv->m_vecViewAngles.y,
	    mv->m_vecViewAngles.z,
	    spt_playerio.m_vecPunchAngle.GetValue().x,
	    spt_playerio.m_vecPunchAngle.GetValue().y,
	    spt_playerio.m_vecPunchAngle.GetValue().z,
	    mv->m_vecVelocity.x,
	    mv->m_vecVelocity.y,
	    mv->m_vecVelocity.z,
	    mv->m_flForwardMove,
	    mv->m_flSideMove,
	    mv->m_flUpMove,
	    mv->m_flMaxSpeed);

	spt_tas.ORIG_ProcessMovement(thisptr, edx, pPlayer, pMove);

	spt_taslogging.Log(
	    "POST ProcessMovement: POS (%.8f, %.8f, %.8f), ANG (%.8f, %.8f, %.8f), PUNCH (%.8f, %.8f, %.8f), VEL (%.8f, %.8f, %.8f), MOVE (%.8f, %.8f, %.8f), MAX %.8f",
	    mv->GetAbsOrigin().x,
	    mv->GetAbsOrigin().y,
	    mv->GetAbsOrigin().z,
	    mv->m_vecViewAngles.x,
	    mv->m_vecViewAngles.y,
	    mv->m_vecViewAngles.z,
	    spt_playerio.m_vecPunchAngle.GetValue().x,
	    spt_playerio.m_vecPunchAngle.GetValue().y,
	    spt_playerio.m_vecPunchAngle.GetValue().z,
	    mv->m_vecVelocity.x,
	    mv->m_vecVelocity.y,
	    mv->m_vecVelocity.z,
	    mv->m_flForwardMove,
	    mv->m_flSideMove,
	    mv->m_flUpMove,
	    mv->m_flMaxSpeed);
}

// Remove keycodes from toggles
static void RemoveKeycodes(CCommand& command)
{
	const char* cmd = command.Arg(0);

	if (cmd && (cmd[0] == '+' || cmd[0] == '-'))
	{
		char BUFFER[128];
		strncpy(BUFFER, cmd, sizeof(BUFFER));
		command.Reset();
		command.Tokenize(BUFFER);
	}
}

bool __fastcall NewTASFeature::HOOKED_CCommandBuffer__DequeueNextCommand(CCommandBuffer* thisptr, int edx)
{
	bool rval = spt_tas.ORIG_CCommandBuffer__DequeueNextCommand(thisptr, edx);
	if (rval)
	{
		CCommand& command = const_cast<CCommand&>(thisptr->GetCommand());

		if (tas_state.controller.IsRecording() && IsRecordable(command))
		{
			RemoveKeycodes(command);
			const char* cmd = ToString(command);
			tas_state.controller.OnCommandExecuted(cmd);
		}
	}

	return rval;
}

CON_COMMAND(tas_freetimes, "print freetimes")
{
	for (int i = 0; i < 2048; ++i)
	{
		auto ent = interfaces::engine_server->PEntityOfEntIndex(i);
		if (ent)
		{
			Msg("%d: %f\n", i, ent->freetime);
		}
	}
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
	tas_state.controller.Play();
}

CON_COMMAND(tas_skip, "Skip to tick in TAS.")
{
	/*auto error = tas_state.controller.Skip();
	if (error.m_bError)
	{
		Warning(error.m_sMessage.c_str());
	}*/
}

CON_COMMAND(tas_record_start, "Starts recording a TAS.")
{
	tas_state.controller.Record_Start();
}

CON_COMMAND(tas_record_stop, "Stops a TAS recording.")
{
	tas_state.controller.Record_Stop();
}

CON_COMMAND(tas_pause, "Pauses TAS playback.")
{
	auto error = tas_state.controller.Pause();
	if(error.m_bError)
	{
		Warning(error.m_sMessage.c_str());
	}
}

CON_COMMAND(tas_stop, "Stops TAS playback.")
{
	auto error = tas_state.controller.Stop();
	if (error.m_bError)
	{
		Warning(error.m_sMessage.c_str());
	}
}

void NewTASFeature::LoadFeature()
{
	if (FrameSignal.Works)
	{
		tas_state.controller.m_fExecConCmd = EngineConCmd;
		tas_state.controller.m_fResetView = []() { tas_state.ResetView(); };
		tas_state.controller.m_fSetView = [](float* pos, float* ang) { tas_state.SetView(pos, ang); };

		FrameSignal.Connect(this, &NewTASFeature::OnFrame);
		InitCommand(tas_init);
		InitCommand(tas_load);
		InitCommand(tas_stop);
		InitCommand(tas_play);
		InitCommand(tas_record_start);
		InitCommand(tas_record_stop);
		InitCommand(tas_save);
		InitCommand(tas_skip);
		InitConcommandBase(tas_loop_cmd);
		InitConcommandBase(tas_record_optimizebulks);
		InitToggle(tas_forward);
		InitToggle(tas_backward);
	}

	InitCommand(tas_freetimes);

	if(CViewRender__RenderViewSignal.Works)
	{
		CViewRender__RenderViewSignal.Connect(&tas_state, &TASState::SetupView);
	}

	if (ORIG__Host_RunFrame)
	{
		pHost_Frametime = *reinterpret_cast<float**>((uintptr_t)ORIG__Host_RunFrame + 227);
}
	else
	{
		pHost_Frametime = nullptr;
	}

	if (ORIG_Host_AccumulateTime)
	{
		pHost_Realtime = *reinterpret_cast<float**>((uintptr_t)ORIG_Host_AccumulateTime + 5);
	}
	else
	{
		pHost_Realtime = nullptr;
	}

	if (ORIG_Host_AccumulateTime && ORIG__Host_RunFrame)
	{
		InitCommand(tas_pause);
	}

	AddHudCallback(
	    "TAS",
	    [this]()
	    {
		    spt_hud.DrawTopHudElement(L"TAS tick: %d", tas_state.controller.m_iCurrentTick);
		    spt_hud.DrawTopHudElement(L"TAS playback tick: %d", tas_state.controller.m_iCurrentPlaybackTick);
	    },
	    tas_hud);
}