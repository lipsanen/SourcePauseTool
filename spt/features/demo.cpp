#include "stdafx.h"
#include "demo.hpp"
#include "..\feature.hpp"
#include "..\OrangeBox\scripts\srctas_reader.hpp"
#include "dbg.h"

DemoStuff g_Demostuff;

void DemoStuff::Demo_StopRecording()
{
	g_Demostuff.HOOKED_Stop();
}

int DemoStuff::Demo_GetPlaybackTick() const
{
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall***)(void*)>(demoplayer))[GetPlaybackTick_Offset](demoplayer);
}

int DemoStuff::Demo_GetTotalTicks() const
{
	if (!pDemoplayer)
		return 0;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<int(__fastcall***)(void*)>(demoplayer))[GetTotalTicks_Offset](demoplayer);
}

bool DemoStuff::Demo_IsPlayingBack() const
{
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall***)(void*)>(demoplayer))[IsPlayingBack_Offset](demoplayer);
}

bool DemoStuff::Demo_IsPlaybackPaused() const
{
	if (!pDemoplayer)
		return false;
	auto demoplayer = *pDemoplayer;
	return (*reinterpret_cast<bool(__fastcall***)(void*)>(demoplayer))[IsPlaybackPaused_Offset](demoplayer);
}

bool DemoStuff::Demo_IsAutoRecordingAvailable() const
{
	return (ORIG_StopRecording && ORIG_SetSignonState);
}

bool DemoStuff::ShouldLoadFeature()
{
	return true;
}

void DemoStuff::InitHooks()
{
	auto callback = PATTERN_CALLBACK
	{
		// Move 1 byte since the pattern starts a byte before the function
		if (ORIG_SetSignonState)
			ORIG_SetSignonState = (_SetSignonState)((uint32_t)ORIG_SetSignonState + 1);
	};

	auto stoprecording_callback = PATTERN_CALLBACK
	{
		// CDemoRecorder::StopRecording
		if (ORIG_StopRecording)
		{
			if (index == 0)
			{
				m_bRecording_Offset = *(int*)((uint32_t)ORIG_StopRecording + 65);
				m_nDemoNumber_Offset = *(int*)((uint32_t)ORIG_StopRecording + 72);
			}
			else if (index == 1)
			{
				m_bRecording_Offset = *(int*)((uint32_t)ORIG_StopRecording + 70);
				m_nDemoNumber_Offset = *(int*)((uint32_t)ORIG_StopRecording + 77);
			}
			else
			{
				Warning("StopRecording had no matching offset clause.\n");
			}

			DevMsg("Found CDemoRecorder offsets m_nDemoNumber %d, m_bRecording %d.\n",
			       m_nDemoNumber_Offset,
			       m_bRecording_Offset);
		}
	};

	auto stop_callback = PATTERN_CALLBACK
	{
		if (ORIG_Record)
		{
			if (index == 0)
			{
				pDemoplayer = *reinterpret_cast<void***>(ORIG_Record + 132);

				// vftable offsets
				GetPlaybackTick_Offset = 2;
				GetTotalTicks_Offset = 3;
				IsPlaybackPaused_Offset = 5;
				IsPlayingBack_Offset = 6;
			}
			else if (index == 1)
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
	};

	HOOK_FUNCTION_WITH_CALLBACK(engine, StopRecording, stoprecording_callback);
	HOOK_FUNCTION_WITH_CALLBACK(engine, SetSignonState, callback);
	HOOK_FUNCTION_WITH_CALLBACK(engine, Stop, stop_callback);
}

void DemoStuff::LoadFeature()
{
	currentAutoRecordDemoNumber = 1;
	isAutoRecordingDemo = false;
	if (!ORIG_Record)
	{
		Warning("y_spt_pause_demo_on_tick is not available.\n");
	}

	if (!ORIG_StopRecording || !ORIG_SetSignonState)
	{
		Warning(
		    "TAS demo recording may overwrite demos if level transitions and saveloads are present in the same script.\n");
	}
	else if (!ORIG_Stop)
	{
		Warning("Manually stopping a TAS demo recording won't stop autorecording.\n");
	}
}

void DemoStuff::UnloadFeature() {}

void __fastcall DemoStuff::HOOKED_StopRecording(void* thisptr, int edx)
{ // This hook will get called twice per loaded save (in most games/versions, at least, according to SAR people), once with m_bLoadgame being false and the next one being true
	if (!scripts::g_TASReader.IsExecutingScript())
	{
		g_Demostuff.ORIG_StopRecording(thisptr, edx);
		g_Demostuff.isAutoRecordingDemo = false;
		g_Demostuff.currentAutoRecordDemoNumber = 1;
		return;
	}

	bool* pM_bRecording = (bool*)((uint32_t)thisptr + g_Demostuff.m_bRecording_Offset);
	int* pM_nDemoNumber = (int*)((uint32_t)thisptr + g_Demostuff.m_nDemoNumber_Offset);

	// This will set m_nDemoNumber to 0 and m_bRecording to false
	g_Demostuff.ORIG_StopRecording(thisptr, edx);

	if (g_Demostuff.isAutoRecordingDemo)
	{
		*pM_nDemoNumber = g_Demostuff.currentAutoRecordDemoNumber;
		*pM_bRecording = true;
	}
	else
	{
		g_Demostuff.currentAutoRecordDemoNumber = 1;
	}
}

void __fastcall DemoStuff::HOOKED_SetSignonState(void* thisptr, int edx, int state)
{
	// This hook only makes sense if StopRecording is also properly hooked
	if (g_Demostuff.ORIG_StopRecording && scripts::g_TASReader.IsExecutingScript())
	{
		bool* pM_bRecording = (bool*)((uint32_t)thisptr + g_Demostuff.m_bRecording_Offset);
		int* pM_nDemoNumber = (int*)((uint32_t)thisptr + g_Demostuff.m_nDemoNumber_Offset);

		// SIGNONSTATE_SPAWN (5): ready to receive entity packets
		// SIGNONSTATE_FULL may be called twice on a load depending on the game and on specific situations. Using SIGNONSTATE_SPAWN for demo number increase instead
		if (state == 5 && g_Demostuff.isAutoRecordingDemo)
		{
			g_Demostuff.currentAutoRecordDemoNumber++;
		}
		// SIGNONSTATE_FULL (6): we are fully connected, first non-delta packet received
		// Starting a demo recording will call this function with SIGNONSTATE_FULL
		// After a load, the engine's demo recorder will only start recording when it reaches this state, so this is a good time to set the flag if needed
		else if (state == 6)
		{
			// Changing sessions may put the recording flag down
			// Start recording again
			if (g_Demostuff.isAutoRecordingDemo)
			{
				*pM_bRecording = true;
			}

			// We may have just started the first recording, so set our autorecording flag and take control over the demo number
			if (*pM_bRecording)
			{
				g_Demostuff.isAutoRecordingDemo = true;
				*pM_nDemoNumber = g_Demostuff.currentAutoRecordDemoNumber;
			}
		}
	}
	g_Demostuff.ORIG_SetSignonState(thisptr, edx, state);
}

void __cdecl DemoStuff::HOOKED_Stop()
{
	g_Demostuff.isAutoRecordingDemo = false;
	if (g_Demostuff.ORIG_Stop)
		g_Demostuff.ORIG_Stop();
}
