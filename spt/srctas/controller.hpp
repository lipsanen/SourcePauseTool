#pragma once

#include "platform.hpp"
#include "script.hpp"
#include "utils.hpp"
#include <functional>
#include <string>

namespace srctas
{
	extern DLL_EXPORT const int NOT_PLAYING;
	extern DLL_EXPORT const int PLAY_TO_END;

	struct DLL_EXPORT FrameBulkState
	{
		FrameBulk* m_sCurrent;
		int m_iTickInBulk;
	};

	struct MoveHistory
	{
		float pos[3];
		float ang[3];
		bool valid = false;
	};

	// Add error handling stuff to interface
	class DLL_EXPORT ScriptController
	{
	public:
		ScriptController();

		Error InitEmptyScript(const char* filepath);
		Error SaveToFile(const char* filepath = nullptr);
		Error LoadFromFile(const char* filepath);
		Error SetToTick(int tick);
		std::string GetCommandForCurrentTick(Error& error);
		std::string GetFrameBulkHistory(int length, Error& error);
		int GetCurrentTick(Error& error);
		int GetTotalTicks(Error& error);
		bool LastTick();
		FrameBulkState GetCurrentFramebulk();
		Error Play();
		Error Pause();
		Error Record_Start();
		Error Record_Stop();
		Error Skip(int tick, float timescale=9999);
		Error Stop();
		bool ShouldPause();
		int GetPlayState();
		bool IsRecording();
		Error TEST_Advance(int ticks);

		Error OnFrame();
		Error OnMove(float pos[3], float ang[3]);
		Error OnCommandExecuted(const char* commandsExecuted);

		Script m_sScript;
		std::string m_sFilepath;
		int m_iCurrentTick = 0;
		int m_iCurrentPlaybackTick = 0;
		int m_iLastValidTick = 0;
		int m_iTickInBulk = 0;
		int m_iCurrentFramebulkIndex = 0;
		int m_iTargetTick = -2;
		bool m_bScriptInit = false;
		bool m_bPaused = false;
		bool m_bAutoPause = true;
		std::vector<MoveHistory> m_vecMoves;
		std::function<void(const char*)> m_fExecConCmd = nullptr;
		std::function<void(float)> m_fSetTimeScale = nullptr;
		std::function<void(float*, float*)> m_fSetView = nullptr;
		std::function<void()> m_fResetView = nullptr;
		std::function<int()> m_fRewindState = nullptr;

		void SetPaused(bool paused) { m_bPaused = paused; }

	private:
		Error Advance(int ticks);
		Error OnFrame_Playing();
		Error OnFrame_Recording();
		Error OnFrame_Paused();
		void OnFrame_HandleEdits();
		void _ResetState();
		void _ForwardAdvance(int ticks);
		void _BackwardAdvance(int ticks);
	};
} // namespace srctas