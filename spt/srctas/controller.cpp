#include "stdafx.h"
#include "controller.hpp"
#include "utils.hpp"
#include <algorithm>

namespace srctas
{
	const int NOT_PLAYING = -2;
	const int PLAYING = -1;

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
		m_iTargetTick = NOT_PLAYING;
		m_iTickInBulk = 0;
		m_iCurrentFramebulkIndex = 0;
		m_iLastValidTick = 0;
		m_fSetTimeScale(1);
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

	Error ScriptController::OnCommandExecuted(const char* commandsExecuted)
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
		auto state = GetCurrentFramebulk();

		if(m_iCurrentTick != m_iCurrentPlaybackTick)
		{
			m_iLastValidTick = std::min(m_iLastValidTick, m_iCurrentTick); // Current playback no longer valid
		}

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
		srctas::Error error;
		int ticks = GetTotalTicks(error);
		if(error.m_bError)
			return error;
		error = Skip(ticks, 1);
		return error;
	}

	Error ScriptController::Pause(PauseState state)
	{
		CHECK_INIT();
		Error error;

		bool paused;

		if(state == PauseState::Auto)
			paused = ShouldPause();
		else
			paused = state == PauseState::Paused;

		if (paused)
		{
			int ticks = GetTotalTicks(error);
			if(error.m_bError)
				return error;
			error = Skip(ticks, 1);
		}
		else
		{
			m_iTargetTick = m_iCurrentTick;
		}

		return error;
	}

	Error ScriptController::Record_Start()
	{
		CHECK_INIT();
		m_iTargetTick = PLAYING;
		return Error();
	}

	Error ScriptController::Record_Stop()
	{
		CHECK_INIT();
		m_iTargetTick = m_iCurrentTick;
		return Error();
	}

	Error ScriptController::Skip(int tick, float timescale)
	{
		CHECK_INIT();

		if(tick < m_iCurrentPlaybackTick || (m_iLastValidTick < tick && m_iLastValidTick != m_iCurrentPlaybackTick))
		{
			_ResetState();
		}
		else if(m_iCurrentTick != m_iCurrentPlaybackTick)
		{
			SetToTick(m_iCurrentPlaybackTick);
		}

		m_iTargetTick = tick;
		m_fSetTimeScale(timescale);

		return Error();
	}

	Error ScriptController::Stop()
	{
		CHECK_INIT();
		_ResetState();
		return Error();
	}

	Error ScriptController::OnFrame(PauseState pauseState)
	{
		Error err;
		if (!m_bScriptInit)
		{
			m_fResetView();
		}
		else
		{
			int state = GetPlayState();

			if (pauseState == PauseState::Unpaused || state > 0)
			{
				err = OnFrame_Playing();
			}
			else if (state <= 0)
			{
				err = OnFrame_Paused();
			}

			if(m_iCurrentTick != m_iCurrentPlaybackTick && m_iCurrentTick < m_vecMoves.size() && m_vecMoves[m_iCurrentTick].valid)
			{
				m_fSetView(m_vecMoves[m_iCurrentTick].pos, m_vecMoves[m_iCurrentTick].ang);
			}
			else
			{
				m_fResetView();
			}
		}

		return err;
	}

	void ScriptController::OnFrame_HandleEdits()
	{
		if(m_iLastValidTick <= m_iCurrentTick && m_iLastValidTick < m_iCurrentPlaybackTick)
		{
			int target = m_iTargetTick;
			_ResetState();
			m_iTargetTick = target;
		}
	}

	bool ScriptController::IsRecording(PauseState pauseState)
	{
		if(!m_bScriptInit)
			return false;

		Error err;
		bool paused;
		if(pauseState == PauseState::Auto)
			paused = ShouldPause();
		else
			paused = pauseState == PauseState::Paused;

		if(!paused)
		{
			return m_iTargetTick == PLAYING;
		}
		else
		{
			return true;
		}
	}

	Error ScriptController::TEST_Advance(int ticks)
	{
		return Advance(ticks);
	}

	Error ScriptController::OnFrame_Playing()
	{
		OnFrame_HandleEdits();

		if(m_iCurrentTick < m_iCurrentPlaybackTick)
		{
			Advance(1);
			return Error();
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

		m_iCurrentPlaybackTick += 1;
		m_iLastValidTick = m_iCurrentPlaybackTick;
		Advance(1);

		if (m_iTargetTick <= m_iCurrentTick && m_iTargetTick >= 0)
		{
			m_fSetTimeScale(1);
			return Pause(PauseState::Unpaused);
		}

		return error;
	}

	Error ScriptController::OnFrame_Recording()
	{
		m_iCurrentPlaybackTick += 1;
		m_iLastValidTick = m_iCurrentPlaybackTick;
		Advance(1);
		return Error();
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
		int startIndex = m_vecMoves.size();
		m_vecMoves.resize(m_iCurrentTick + 1);
		for(int i=startIndex; i <= m_iCurrentTick; ++i)
		{
			m_vecMoves[i] = MoveHistory();
		}

		for(int i=0; i < 3; ++i)
		{
			m_vecMoves[m_iCurrentTick].ang[i] = ang[i];
			m_vecMoves[m_iCurrentTick].pos[i] = pos[i];
		}

		m_vecMoves[m_iCurrentTick].valid = true;
		return Error();
	}

	int ScriptController::GetPlayState()
	{
		int rewind = m_fRewindState();

		if(rewind != 0)
		{
			return rewind;
		}

		if(m_iTargetTick == NOT_PLAYING)
		{
			return 0;
		}

		if(m_iCurrentTick != m_iCurrentPlaybackTick)
		{
			return 0;
		}
		else if(m_iCurrentTick == m_iTargetTick)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}

	bool ScriptController::ShouldPause()
	{
		if(!m_bScriptInit)
			return false;

		int state = GetPlayState();

		if(m_iCurrentPlaybackTick == 0 && m_iTargetTick == NOT_PLAYING)
		{
			return false;
		}

		if(m_iCurrentTick < m_iCurrentPlaybackTick)
		{
			return true;
		}
		else
		{
			return state <= 0;
		}

	}
} // namespace srctas