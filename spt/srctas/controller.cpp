#include "stdafx.h"
#include "controller.hpp"
#include "utils.hpp"
#include <algorithm>

namespace srctas
{
	#define CHECK_INIT() if(!m_bScriptInit) \
	{ \
		Error err; \
		err.m_bError = true; \
		err.m_sMessage = "No script initialized"; \
		return err; \
	}

	ScriptController::ScriptController()
	{
		m_fExecConCmd = [](auto a) {};
		m_fSetTimeScale = [](auto a) {};
		m_fSetView = [](auto a, auto b) {};
		m_fResetView = []() {};
		m_fRewindState = []() { return 0; };
	}

	Error ScriptController::InitEmptyScript(const char* filepath)
	{
		Error error;
		m_sScript.Clear();
		_ResetState();
		m_sFilepath = filepath;
		m_bScriptInit = true;
		return error;
	}

	Error ScriptController::SaveToFile(const char* filepath)
	{
		CHECK_INIT();

		if (filepath)
		{
			m_sFilepath = filepath;
		}

		Error error;

		if (m_sFilepath.empty())
		{
			error.m_bError = true;
			error.m_sMessage = "No script loaded, cannot save to file.";
			return error;
		}
		else
		{
			error = m_sScript.WriteToFile(m_sFilepath.c_str());
		}

		return error;
	}

	Error ScriptController::LoadFromFile(const char* filepath)
	{
		Error error = Script::ParseFrom(filepath, &m_sScript);
		if (!error.m_bError)
		{
			m_sFilepath = filepath;
			m_bScriptInit = true;
		}
		else
		{
			// Load failed, we might be in some weird half loaded state so clear everything
			m_sFilepath = ""; 
			_ResetState();
			m_sScript.Clear();
			m_bScriptInit = false;
		}
		return error;
	}

	Error ScriptController::SetToTick(int tick)
	{
		CHECK_INIT();

		Error error;
		m_iCurrentTick = 0;
		m_iTickInBulk = 0;
		m_iCurrentFramebulkIndex = 0;

		if (tick < 0)
		{
			int totalTicks = GetTotalTicks(error);

			if (error.m_bError)
			{
				return error;
			}
			else if (totalTicks < tick)
			{
				error.m_bError = true;
			}

			tick = totalTicks - tick;
		}

		Advance(tick);

		return error;
	}

	Error ScriptController::Advance(int ticks)
	{
		CHECK_INIT();

		Error error;
		error.m_bError = false;

		if (ticks > 0)
		{
			_ForwardAdvance(ticks);
		}
		else if (ticks < 0)
		{
			_BackwardAdvance(-ticks);
		}

		return error;
	}

	void ScriptController::_ResetState()
	{
		m_iCurrentPlaybackTick = 0;
		m_iCurrentTick = 0;
		m_iTargetTick = -1;
		m_iTickInBulk = 0;
		m_iCurrentFramebulkIndex = 0;
		m_fSetTimeScale(1);
		m_bPaused = false;
		m_bRecording = false;
		m_bPlayingTAS = false;
	}

	void ScriptController::_ForwardAdvance(int ticks)
	{
		int ticksAdvanced = 0;

		while (ticksAdvanced < ticks)
		{
			if (static_cast<std::size_t>(m_iCurrentFramebulkIndex + 1) < m_sScript.m_vFrameBulks.size()
			    && m_iTickInBulk == m_sScript.m_vFrameBulks[m_iCurrentFramebulkIndex].m_iTicks - 1)
			{
				m_iTickInBulk = 0;
				++m_iCurrentFramebulkIndex;
			}
			else
			{
				++m_iTickInBulk;
			}

			++m_iCurrentTick;
			++ticksAdvanced;
		}
	}

	void ScriptController::_BackwardAdvance(int ticks)
	{
		SetToTick(std::max(0, m_iCurrentTick - ticks));
	}

	Error ScriptController::AddCommands(const char* commandsExecuted)
	{
		CHECK_INIT();

		Error error;

		if (m_iCurrentFramebulkIndex < 0)
		{
			error.m_sMessage = "Tried to add commands while no framebulk selected";
			error.m_bError = true;
			return error;
		}

		if (m_sScript.m_vFrameBulks.size() == 0)
		{
			// Add first framebulk
			if (m_iCurrentTick > 0)
			{
				// Need to add an empty framebulk in the beginning
				FrameBulk bulk;
				bulk.m_iTicks = m_iCurrentTick;
				m_sScript.m_vFrameBulks.push_back(bulk);
				m_sScript.m_vFrameBulks.push_back(FrameBulk());
			}
			else
			{
				m_sScript.m_vFrameBulks.push_back(FrameBulk());
			}
		}
		else
		{
			int ticksInCurrentBulk = m_sScript.m_vFrameBulks[m_iCurrentFramebulkIndex].m_iTicks;

			if (m_iTickInBulk >= ticksInCurrentBulk)
			{
				m_sScript.m_vFrameBulks[m_iCurrentFramebulkIndex].m_iTicks =
				    m_iTickInBulk; // Extend the previous tick bulk to this one
				m_sScript.m_vFrameBulks.push_back(FrameBulk());
			}
			else if (m_iTickInBulk > 0)
			{
				// Not first tick in bulk
				int remainingTicks =
				    ticksInCurrentBulk - m_iTickInBulk; // This many ticks for the newly created bulk
				m_sScript.m_vFrameBulks[m_iCurrentFramebulkIndex].m_iTicks =
				    m_iTickInBulk; // Fix the old tick count

				FrameBulk bulk;
				bulk.m_iTicks = remainingTicks;
				auto iterator = m_sScript.m_vFrameBulks.begin() + m_iCurrentFramebulkIndex + 1;
				m_sScript.m_vFrameBulks.insert(iterator, bulk);
			}
		}

		// Use this sledgehammer to fix the bulk indexing.
		// Could be done faster, but this is the most idiot proof way
		SetToTick(m_iCurrentTick);
		m_iCurrentPlaybackTick = m_iCurrentTick; // Set the playback tick to current tick, script has been modified
		auto state = GetCurrentFramebulk();
		if (state.m_sCurrent == nullptr)
		{
			error.m_bError = true;
			error.m_sMessage =
			    "Controller state machine broke, can't find the framebulk to add commands to";
		}
		else
		{
			state.m_sCurrent->ApplyCommand(commandsExecuted);
		}

		return error;
	}

	std::string ScriptController::GetCommandForCurrentTick(Error& error)
	{
		std::string output;

		if (!m_bScriptInit || m_sScript.m_vFrameBulks.empty())
		{
			return ""; // No bulks or no bulk selected
		}
		else if (m_iTickInBulk == 0)
		{
			// Only execute things on the first tick in the bulk
			output = m_sScript.m_vFrameBulks[m_iCurrentFramebulkIndex].GetCommand();
		}
		// Otherwise return empty string, nothing to be executed

		return output;
	}

	std::string ScriptController::GetFrameBulkHistory(int length, Error& error)
	{
		if (!m_bScriptInit)
		{
			return "";
		}

		std::string output;
		int start = std::max(0, m_iCurrentFramebulkIndex - length);

		for (int i = start;
		     i <= m_iCurrentFramebulkIndex && static_cast<std::size_t>(i) < m_sScript.m_vFrameBulks.size();
		     ++i)
		{
			auto& framebulk = m_sScript.m_vFrameBulks[i];
			if (!output.empty())
			{
				output += '\n';
			}

			output += framebulk.GetFramebulkString();
		}

		return output;
	}

	int ScriptController::GetCurrentTick(Error& error)
	{
		if(m_bScriptInit)
			return m_iCurrentTick;
		else
			return 0;
	}

	int ScriptController::GetTotalTicks(Error& error)
	{
		int ticks = 0;

		for (auto& bulk : m_sScript.m_vFrameBulks)
		{
			ticks += bulk.m_iTicks;
		}

		return ticks;
	}

	bool ScriptController::LastTick()
	{
		Error err;
		return GetCurrentTick(err) >= GetTotalTicks(err);
	}

	FrameBulkState ScriptController::GetCurrentFramebulk()
	{
		FrameBulkState state;
		if (m_iCurrentFramebulkIndex < 0
		    && static_cast<std::size_t>(m_iCurrentFramebulkIndex) >= m_sScript.m_vFrameBulks.size())
		{
			state.m_iTickInBulk = 0;
			state.m_sCurrent = nullptr;
		}
		else
		{
			state.m_iTickInBulk = m_iTickInBulk;
			state.m_sCurrent = &m_sScript.m_vFrameBulks[m_iCurrentFramebulkIndex];
		}

		return state;
	}

	Error ScriptController::Play()
	{
		CHECK_INIT();
		_ResetState();
		m_bPlayingTAS = true;
		return Error();
	}

	Error ScriptController::Pause()
	{
		CHECK_INIT();
		Error error;

		if (!m_bPlayingTAS && !m_bRecording)
		{
			error.m_sMessage = "Not playing a TAS or recording, cannot pause\n";
			error.m_bError = true;
			return error;
		}

		if (m_bPaused)
		{
			m_bPaused = false;

			if (!m_bRecording)
			{
				m_bPlayingTAS = true;
			}
		}
		else
		{
			m_bPaused = true;
		}

		return Error();
	}

	Error ScriptController::Record_Start()
	{
		CHECK_INIT();
		SetToTick(m_iCurrentPlaybackTick);
		m_bRecording = true;
		return Error();
	}

	Error ScriptController::Record_Stop()
	{
		CHECK_INIT();
		SetToTick(m_iCurrentPlaybackTick);
		m_bRecording = false;
		return Error();
	}

	Error ScriptController::Skip(int tick)
	{
		CHECK_INIT();
		_ResetState();
		m_bPlayingTAS = true;
		m_iTargetTick = tick;
		m_fSetTimeScale(9999);
		return Error();
	}

	Error ScriptController::Stop()
	{
		CHECK_INIT();
		_ResetState();
		return Error();
	}

	Error ScriptController::OnFrame()
	{
		if (!m_bScriptInit || m_bPaused || (!m_bPlayingTAS && !m_bRecording))
		{
			return Error();
		}

		if (m_bPlayingTAS && !m_bPaused)
		{
			return OnFrame_Playing();
		}
		else if (m_bPaused)
		{
			return OnFrame_Paused();
		}

		return Error();
	}

	Error ScriptController::OnFrame_Playing()
	{
		if (m_iCurrentTick > m_iCurrentPlaybackTick)
		{
			Skip(m_iCurrentTick); // Trying to playback the TAS when ahead of playback causes it to fast forward to current point
		}
		else
		{
			m_iCurrentPlaybackTick = m_iCurrentTick;
		}

		if (m_iTargetTick == m_iCurrentTick && !m_bPaused)
		{
			m_fSetTimeScale(1);
			return Pause();
		}

		srctas::Error error;
		std::string command = GetCommandForCurrentTick(error);

		if (error.m_bError)
		{
			return error;
		}
		else if (!command.empty())
		{
			m_fExecConCmd(command.c_str());
		}

		if (m_bPlayingTAS && LastTick())
		{
			m_bPlayingTAS = false;
		}
		else
		{
			m_iCurrentPlaybackTick += 1;
			Advance(1);
		}

		return error;
	}

	Error ScriptController::OnFrame_Paused()
	{
		int state = m_fRewindState();

		if (state < 0)
		{
			Advance(-1);
		}
		else if (state > 0)
		{
			Advance(1);
		}

		return Error();
	}

	Error ScriptController::OnMove(float pos[3], float ang[3])
	{
		return Error();
	}
} // namespace srctas