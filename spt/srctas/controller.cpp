#include "stdafx.h"
#include "controller.hpp"
#include "utils.hpp"
#include <algorithm>

namespace srctas
{
	void ScriptController::SetCallbacks(std::function<void(const char*)> execConCmd)
	{
		this->execConCmd = execConCmd;
		this->m_bCallbacksSet = true;
	}

	Error ScriptController::InitEmptyScript(const char* filepath)
	{
		Error error;
		m_sScript.Clear();
		SetToTick(0);
		m_sFilepath = filepath;
		return error;
	}

	Error ScriptController::SaveToFile(const char* filepath)
	{
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
		}
		else
		{
			m_sFilepath = ""; // Load failed, we might be in some weird half loaded state
		}
		return error;
	}

	Error ScriptController::SetToTick(int tick)
	{
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

		if (m_iCurrentFramebulkIndex < 0 || m_sScript.m_vFrameBulks.empty())
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
		return m_iCurrentTick;
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
		SetToTick(0);
		m_bPlayingTAS = true;
		m_bPaused = false;
		m_bRecording = false;
		return Error();
	}

	Error ScriptController::Pause()
	{
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
		m_bRecording = true;
		return Error();
	}

	Error ScriptController::Record_Stop()
	{
		m_bRecording = false;
		return Error();
	}

	Error ScriptController::Skip()
	{
		return Error();
	}

	Error ScriptController::Stop()
	{
		m_bPlayingTAS = false;
		m_bRecording = false;
		m_bPaused = false;
		return Error();
	}

	Error ScriptController::OnFrame()
	{
		if (!m_bCallbacksSet || m_bPaused || (!m_bPlayingTAS && !m_bRecording))
		{
			return Error();
		}

		if (m_bPlayingTAS)
		{
			srctas::Error error;
			std::string command = GetCommandForCurrentTick(error);

			if (error.m_bError)
			{
				return error;
			}
			else
			{
				execConCmd(command.c_str());
			}
		}

		if (m_bPlayingTAS && LastTick())
		{
			m_bPlayingTAS = false;
		}
		else
		{
			Advance(1);
		}

		return Error();
	}
} // namespace srctas