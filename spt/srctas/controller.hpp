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

	// Add error handling stuff to interface
	class __declspec(dllexport) ScriptController
	{
	public:

		void SetCallbacks(std::function<void(const char*)> execConCmd);

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
		Error Skip();
		Error Stop();
		Error OnFrame();

		Script m_sScript;
		std::string m_sFilepath;
		int m_iCurrentTick = -1;
		int m_iTickInBulk = -1;
		int m_iCurrentFramebulkIndex = -1;
		bool m_bPlayingTAS = false;
		bool m_bRecording = false;
		bool m_bPaused = false;
		std::function<void(const char*)> execConCmd = nullptr;
		bool m_bCallbacksSet = false;

	private:
		void _ForwardAdvance(int ticks);
		void _BackwardAdvance(int ticks);
	};
} // namespace srctas