#include "stdafx.h"

#include "EngineDLL.hpp"

#include <SPTLib\hooks.hpp>
#include <SPTLib\memutils.hpp>

#include "..\..\sptlib-wrapper.hpp"
#include "..\..\utils\ent_utils.hpp"
#include "..\cvars.hpp"
#include "..\modules.hpp"
#include "..\overlay\overlay-renderer.hpp"
#include "..\patterns.hpp"
#include "..\scripts\srctas_reader.hpp"
#include "..\..\features\afterframes.hpp"

using std::size_t;
using std::uintptr_t;

void __fastcall EngineDLL::HOOKED_StopRecording(void* thisptr, int edx)
{
	TRACE_ENTER();
	engineDLL.HOOKED_StopRecording_Func(thisptr, edx);
}

void __fastcall EngineDLL::HOOKED_SetSignonState(void* thisptr, int edx, int state)
{
	TRACE_ENTER();
	engineDLL.HOOKED_SetSignonState_Func(thisptr, edx, state);
}

void __cdecl EngineDLL::HOOKED_Stop()
{
	TRACE_ENTER();
	engineDLL.HOOKED_Stop_Func();
}

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::engine::##name);
#define GET_HOOKEDFUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[engine dll] Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			patternContainer.AddHook(HOOKED_##future_name, (PVOID*)&ORIG_##future_name); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::engine::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning("[engine dll] Could not find " #future_name ".\n"); \
		} \
	}

#define GET_FUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[engine dll] Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::engine::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning("[engine dll] Could not find " #future_name ".\n"); \
		} \
	}

void EngineDLL::Hook(const std::wstring& moduleName,
                     void* moduleHandle,
                     void* moduleBase,
                     size_t moduleLength,
                     bool needToIntercept)
{
	Clear(); // Just in case.
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	patternContainer.Init(moduleName);

	uintptr_t ORIG_Record = NULL;

	DEF_FUTURE(Record);
	DEF_FUTURE(CEngineTrace__PointOutsideWorld);
	DEF_FUTURE(StopRecording);
	DEF_FUTURE(SetSignonState);
	DEF_FUTURE(Stop);

	GET_FUTURE(CEngineTrace__PointOutsideWorld);
	GET_HOOKEDFUTURE(StopRecording);
	GET_HOOKEDFUTURE(SetSignonState);
	GET_HOOKEDFUTURE(Stop);

	auto pRecord = fRecord.get();
	if (ORIG_Record)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_Record);

		if (ptnNumber == 0)
		{
			pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 132);

			// vftable offsets
			GetPlaybackTick_Offset = 2;
			GetTotalTicks_Offset = 3;
			IsPlaybackPaused_Offset = 5;
			IsPlayingBack_Offset = 6;
		}
		else if (ptnNumber == 1)
		{
			pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 0xA2);

			// vftable offsets
			GetPlaybackTick_Offset = 3;
			GetTotalTicks_Offset = 4;
			IsPlaybackPaused_Offset = 6;
			IsPlayingBack_Offset = 7;
		}
		else
			Warning(
			    "Record pattern had no matching clause for catching the demoplayer. y_spt_pause_demo_on_tick unavailable.\n");

		DevMsg("Found demoplayer at %p, record is at %p.\n", pDemoplayer, ORIG_Record);
	}
	else
	{
		Warning("y_spt_pause_demo_on_tick is not available.\n");
	}

	// CDemoRecorder::StopRecording
	if (ORIG_StopRecording)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_StopRecording);

		if (ptnNumber == 0)
		{
			m_bRecording_Offset = *(int*)((uint32_t)ORIG_StopRecording + 65);
			m_nDemoNumber_Offset = *(int*)((uint32_t)ORIG_StopRecording + 72);
		}
		else if (ptnNumber == 1)
		{
			m_bRecording_Offset = *(int*)((uint32_t)ORIG_StopRecording + 70);
			m_nDemoNumber_Offset = *(int*)((uint32_t)ORIG_StopRecording + 77);
		}

		DevMsg("Found CDemoRecorder offsets m_nDemoNumber %d, m_bRecording %d.\n",
		       m_nDemoNumber_Offset,
		       m_bRecording_Offset);
		if (!ORIG_Stop)
			Warning("Manually stopping a TAS demo recording won't stop autorecording.\n");
	}

	// Move 1 byte since the pattern starts a byte before the function
	if (ORIG_SetSignonState)
		ORIG_SetSignonState = (_SetSignonState)((uint32_t)ORIG_SetSignonState + 1);

	if (!ORIG_StopRecording || !ORIG_SetSignonState)
	{
		Warning(
		    "TAS demo recording may overwrite demos if level transitions and saveloads are present in the same script.\n");
	}

	patternContainer.Hook();
}

void EngineDLL::Unhook()
{
	patternContainer.Unhook();
	Clear();
}

void EngineDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_StopRecording = nullptr;
	ORIG_StopRecording = nullptr;
	ORIG_SetSignonState = nullptr;
	ORIG_Stop = nullptr;
	pGameServer = nullptr;
	pM_bLoadgame = nullptr;
	shouldPreventNextUnpause = false;
	pM_State = nullptr;
	pM_nSignonState = nullptr;
	pDemoplayer = nullptr;
	currentAutoRecordDemoNumber = 1;
}

int EngineDLL::Demo_GetPlaybackTick() const
{
	TRACE_ENTER();
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall***)(void*)>(demoplayer))[GetPlaybackTick_Offset](demoplayer);
}

int EngineDLL::Demo_GetTotalTicks() const
{
	TRACE_ENTER();
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall***)(void*)>(demoplayer))[GetTotalTicks_Offset](demoplayer);
}

bool EngineDLL::Demo_IsPlayingBack() const
{
	TRACE_ENTER();
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall***)(void*)>(demoplayer))[IsPlayingBack_Offset](demoplayer);
}

bool EngineDLL::Demo_IsPlaybackPaused() const
{
	TRACE_ENTER();
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall***)(void*)>(demoplayer))[IsPlaybackPaused_Offset](demoplayer);
}

// Basically a wrapper for the stop command hooked function to expose it to the rest of the code with a nicer name
void EngineDLL::Demo_StopRecording()
{
	HOOKED_Stop_Func();
}

bool EngineDLL::Demo_IsAutoRecordingAvailable() const
{
	return (ORIG_StopRecording && ORIG_SetSignonState);
}

void __fastcall EngineDLL::HOOKED_StopRecording_Func(void* thisptr, int edx)
{
	// This hook will get called twice per loaded save (in most games/versions, at least, according to SAR people), once with m_bLoadgame being false and the next one being true
	if (!scripts::g_TASReader.IsExecutingScript())
	{
		ORIG_StopRecording(thisptr, edx);
		isAutoRecordingDemo = false;
		currentAutoRecordDemoNumber = 1;
		return;
	}

	bool* pM_bRecording = (bool*)((uint32_t)thisptr + m_bRecording_Offset);
	int* pM_nDemoNumber = (int*)((uint32_t)thisptr + m_nDemoNumber_Offset);

	// This will set m_nDemoNumber to 0 and m_bRecording to false
	ORIG_StopRecording(thisptr, edx);

	if (isAutoRecordingDemo)
	{
		*pM_nDemoNumber = currentAutoRecordDemoNumber;
		*pM_bRecording = true;
	}
	else
	{
		currentAutoRecordDemoNumber = 1;
	}
}

void __fastcall EngineDLL::HOOKED_SetSignonState_Func(void* thisptr, int edx, int state)
{
	// This hook only makes sense if StopRecording is also properly hooked
	if (ORIG_StopRecording && scripts::g_TASReader.IsExecutingScript())
	{
		bool* pM_bRecording = (bool*)((uint32_t)thisptr + m_bRecording_Offset);
		int* pM_nDemoNumber = (int*)((uint32_t)thisptr + m_nDemoNumber_Offset);

		// SIGNONSTATE_SPAWN (5): ready to receive entity packets
		// SIGNONSTATE_FULL may be called twice on a load depending on the game and on specific situations. Using SIGNONSTATE_SPAWN for demo number increase instead
		if (state == 5 && isAutoRecordingDemo)
		{
			currentAutoRecordDemoNumber++;
		}
		// SIGNONSTATE_FULL (6): we are fully connected, first non-delta packet received
		// Starting a demo recording will call this function with SIGNONSTATE_FULL
		// After a load, the engine's demo recorder will only start recording when it reaches this state, so this is a good time to set the flag if needed
		else if (state == 6)
		{
			// Changing sessions may put the recording flag down
			// Start recording again
			if (isAutoRecordingDemo)
			{
				*pM_bRecording = true;
			}

			// We may have just started the first recording, so set our autorecording flag and take control over the demo number
			if (*pM_bRecording)
			{
				isAutoRecordingDemo = true;
				*pM_nDemoNumber = currentAutoRecordDemoNumber;
			}
		}
	}
	ORIG_SetSignonState(thisptr, edx, state);
}

void __cdecl EngineDLL::HOOKED_Stop_Func()
{
	isAutoRecordingDemo = false;
	if(ORIG_Stop)
		ORIG_Stop();
}