#pragma once

#include "script.hpp"
#include "utils.hpp"
#include <functional>
#include <string>

namespace srctas
{
	struct __declspec(dllexport) FrameBulkState
	{
		FrameBulk* m_sCurrent;
		int m_iTickInBulk;
	};

	struct MoveHistory
	{
		float pos[3];
		float ang[3];
	};

	// Add error handling stuff to interface
	class __declspec(dllexport) ScriptController
	{
	public:
		ScriptController();

		Error InitEmptyScript(const char* filepath);
		Error SaveToFile(const char* filepath = nullptr);
		Error LoadFromFile(const char* filepath);
		Error SetToTick(int tick);
		Error Advance(int ticks);
		Error AddCommands(const char* commandsExecuted);
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
		Error Skip(int tick);
		Error Stop();
		Error OnFrame();
		Error OnMove(float pos[3], float ang[3]);

		Script m_sScript;
		std::string m_sFilepath;
		int m_iCurrentTick = -1;
		int m_iCurrentPlaybackTick = -1;
		int m_iTickInBulk = -1;
		int m_iCurrentFramebulkIndex = -1;
		int m_iTargetTick = -1;
		bool m_bScriptInit = false;
		bool m_bPlayingTAS = false;
		bool m_bRecording = false;
		bool m_bPaused = false;
		std::function<void(const char*)> m_fExecConCmd = nullptr;
		std::function<void(float)> m_fSetTimeScale = nullptr;
		std::function<void(float*, float*)> m_fSetView = nullptr;
		std::function<void()> m_fResetView = nullptr;
		std::function<int()> m_fRewindState = nullptr;

	private:
		Error OnFrame_Playing();
		Error OnFrame_Paused();
		void _ResetState();
		void _ForwardAdvance(int ticks);
		void _BackwardAdvance(int ticks);
	};
} // namespace srctas