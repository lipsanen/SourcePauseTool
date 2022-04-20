#pragma once

#include "script.hpp"
#include "utils.hpp"
#include <string>

namespace srctas
{
	struct FrameBulkState
	{
		FrameBulk* m_sCurrent;
		int m_iTickInBulk;
	};

	// Add error handling stuff to interface
	class ScriptController
	{
	public:
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

		Script m_sScript;
		std::string m_sFilepath;
		int m_iCurrentTick = -1;
		int m_iTickInBulk = -1;
		int m_iCurrentFramebulkIndex = -1;

	private:
		void _ForwardAdvance(int ticks);
		void _BackwardAdvance(int ticks);
	};
} // namespace srctas