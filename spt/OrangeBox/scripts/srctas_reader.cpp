#include "stdafx.h"

#include "srctas_reader.hpp"
#include "..\..\sptlib-wrapper.hpp"
#include "..\..\utils\string_parsing.hpp"
#include "..\cvars.hpp"
#include "..\spt-serverplugin.hpp"
#include "..\modules\ClientDLL.hpp"
#include "..\modules\EngineDLL.hpp"
#include "..\modules.hpp"
#include "framebulk_handler.hpp"
#include "..\..\utils\math.hpp"
#include "line_types.hpp"

namespace scripts
{
	SourceTASReader g_TASReader;
	const std::string SCRIPT_EXT = ".srctas";

	const char* RESET_VARS[] = {
		"cl_forwardspeed",
		"cl_sidespeed",
		"cl_yawspeed"
	};

	const int RESET_VARS_COUNT = ARRAYSIZE(RESET_VARS);


	SourceTASReader::SourceTASReader()
	{
		InitPropertyHandlers();
		iterationFinished = true;
	}

	void SourceTASReader::ExecuteScriptAndPause(const std::string& script, int pauseTick)
	{
		freezeVariables = false;
		fileName = script;
		CommonExecuteScript(false, pauseTick);
		

		if (pauseTick == UNLIMITED_LENGTH)
			pauseTick = currentScript.GetScriptLength();

		clientDLL.AddIntoAfterframesQueue(afterframes_entry_t(pauseTick, "tas_pause 1"));
		clientDLL.AddIntoAfterframesQueue(afterframes_entry_t(pauseTick, "tas_record_start"));		
	}

	void SourceTASReader::ExecuteScript(const std::string& script)
	{
		freezeVariables = false;
		fileName = script;
		CommonExecuteScript(false, UNLIMITED_LENGTH);
	}

	void SourceTASReader::StartSearch(const std::string& script)
	{
		freezeVariables = false;
		fileName = script;
		CommonExecuteScript(true, UNLIMITED_LENGTH);
		freezeVariables = true;
	}

	void SourceTASReader::ReadScript(bool search)
	{
		std::string gameDir = GetGameDir();
		scriptStream.open(gameDir + "\\" + fileName + SCRIPT_EXT);

		if (!scriptStream.is_open())
			throw std::exception("File does not exist");
		ParseProps();

		if (search && searchType == SearchType::None)
			throw std::exception("In search mode but search property is not set");
		else if (!search && searchType != SearchType::None)
			throw std::exception("Not in search mode but search property is set");

		while (!scriptStream.eof())
		{
			if (IsFramesLine())
				ParseFrames();
			else if (IsVarsLine())
				ParseVariables();
			else
				throw std::exception("Unexpected section order in file. Expected order is props - variables - frames");
		}
	}

	void SourceTASReader::SearchResult(scripts::SearchResult result)
	{
		try
		{
			variables.SetResult(result);
			CommonExecuteScript(true, UNLIMITED_LENGTH);
		}
		catch (const std::exception& ex)
		{
			Msg("Error setting result: %s\n", ex.what());
		}
		catch (const SearchDoneException&)
		{
			Msg("Search done.\n");
			variables.PrintBest();
			iterationFinished = true;
		}
		catch (...)
		{
			Msg("Unexpected exception on line %i\n", currentLine);
		}
	}

	void SourceTASReader::CommonExecuteScript(bool search, int maxLength)
	{
		try
		{
			Reset();
#if OE
			const char* dir = y_spt_gamedir.GetString();
			if (dir == NULL || dir[0] == '\0')
				Msg("WARNING: Trying to load a script file without setting the game directory with y_spt_gamedir in old engine!\n");
#endif
			ReadScript(search);
			Execute(maxLength);
		}
		catch (const std::exception& ex)
		{
			Msg("Error in line %i: %s!\n", currentLine, ex.what());
		}
		catch (const SearchDoneException&)
		{
			Msg("Search done.\n");
			variables.PrintBest();
		}
		catch (...)
		{
			Msg("Unexpected exception on line %i\n", currentLine);
		}

		scriptStream.close();
	}

	void SourceTASReader::OnAfterFrames()
	{
		if (conditions.empty() || iterationFinished)
			return;

		++currentTick;

		bool allTrue = true;

		for (auto& pointer : conditions)
		{
			allTrue = allTrue && pointer->IsTrue(currentTick, currentScript.GetScriptLength());
			if (pointer->ShouldTerminate(currentTick, currentScript.GetScriptLength()))
			{
				iterationFinished = true;
				SearchResult(SearchResult::Fail);

				return;
			}
		}

		if (allTrue)
		{
			iterationFinished = true;
			SearchResult(SearchResult::Success);
		}	
	}

	int SourceTASReader::GetCurrentScriptLength()
	{
		return currentScript.GetScriptLength();
	}

	const ParsedScript & SourceTASReader::GetCurrentScript()
	{
		return currentScript;
	}

	void SourceTASReader::Execute(int maxLength)
	{
		currentTick = 0;
		iterationFinished = false;
		SetFpsAndPlayspeed();
		clientDLL.ResetAfterframesQueue();
		currentScript.Init(maxLength);

		std::string startCmd(currentScript.GetInitCommand() + ";" + currentScript.GetDuringLoadCmd());
		EngineConCmd(startCmd.c_str());
		DevMsg("Executing start command: %s\n", startCmd.c_str());	

		if(!demoName.empty())
			clientDLL.AddIntoAfterframesQueue(afterframes_entry_t(demoDelay, "record " + demoName));

		for (auto& entry : currentScript.GetAfterFramesEntries())
		{
			clientDLL.AddIntoAfterframesQueue(entry);
		}
	}

	void SourceTASReader::SetFpsAndPlayspeed()
	{
		std::ostringstream os;
		tickTime = engineDLL.GetTickrate();
		float fps = 1.0f / tickTime * playbackSpeed;
		os << "host_framerate " << tickTime << "; fps_max " << fps;

		currentScript.AddScriptLine(new PropertyLine(origLine, os.str()));
	}

	bool SourceTASReader::ParseLine()
	{
		if (!scriptStream.good())
		{
			return false;
		}

		std::getline(scriptStream, editedLine);
		origLine = editedLine;
		SetNewLine();

		return true;
	}

	void SourceTASReader::SetNewLine()
	{
		const std::string COMMENT_START = "//";
		int end = editedLine.find(COMMENT_START); // ignore comments
		if (end != std::string::npos)
			editedLine.erase(end);

		ReplaceVariables();
		lineStream.str(editedLine);
		lineStream.clear();
		++currentLine;
	}

	void SourceTASReader::ReplaceVariables()
	{
		for(auto& variable : variables.variableMap)
		{
			ReplaceAll(editedLine, GetVarIdentifier(variable.first), variable.second.GetValue());
		}
	}

	void SourceTASReader::ResetConvars()
	{
#ifndef OE
		auto icvar = GetCvarInterface();

#ifndef P2
		ConCommandBase* cmd = icvar->GetCommands();

		// Loops through the console variables and commands
		while (cmd != NULL)
		{
#else
		ICvar::Iterator iter(icvar);

		for (iter.SetFirst(); iter.IsValid(); iter.Next())
		{
			auto cmd = iter.Get();
#endif

			const char* name = cmd->GetName();
			// Reset any variables that have been marked to be reset for TASes
			if (!cmd->IsCommand() && name != NULL && cmd->IsFlagSet(FCVAR_TAS_RESET))
			{			
				auto convar = icvar->FindVar(name);
				DevMsg("Trying to reset variable %s\n", name);

				// convar found
				if (convar != NULL) 
				{
					DevMsg("Resetting var %s to value %s\n", name, convar->GetDefault());
					convar->SetValue(convar->GetDefault());
				}		
				else
					throw std::exception("Unable to find listed console variable!");
			}

			// Issue minus commands to reset any keypresses
			else if (cmd->IsCommand() && cmd->GetName() != NULL && cmd->GetName()[0] == '-')
			{
				DevMsg("Running command %s\n", cmd->GetName());
				EngineConCmd(cmd->GetName());
			}

#ifndef P2
			cmd = cmd->GetNext();
#endif
		}

		// Reset any variables selected above
		for (int i = 0; i < RESET_VARS_COUNT; ++i)
		{
			auto resetCmd = icvar->FindVar(RESET_VARS[i]);
			if (resetCmd != NULL)
			{
				DevMsg("Resetting var %s to value %s\n", resetCmd->GetName(), resetCmd->GetDefault());
				resetCmd->SetValue(resetCmd->GetDefault());
			}
			else
				DevWarning("Unable to find console variable %s\n", RESET_VARS[i]);
		}
#endif
	}

	void SourceTASReader::Reset()
	{
		if (!freezeVariables)
		{
			variables.Clear();
		}
		ResetIterationState();
	}

	void SourceTASReader::ResetIterationState()
	{
		ResetConvars();
		conditions.clear();
		scriptStream.clear();
		lineStream.clear();
		editedLine.clear();
		currentLine = 0;
		searchType = SearchType::None;
		playbackSpeed = 1.0f;
		demoDelay = 0;
		demoName.clear();
		currentScript.Reset();
	}

	void SourceTASReader::ParseProps()
	{
		while (ParseLine())
		{
			if (IsFramesLine() || IsVarsLine())
			{
				break;
			}
			ParseProp();
		}
	}

	void SourceTASReader::ParseProp()
	{
		if (isLineEmpty())
		{
			return;
		}	

		std::string prop;
		std::string value;
		GetDoublet(lineStream, prop, value, ' ');

		if (propertyHandlers.find(prop) != propertyHandlers.end())
		{
			(this->*propertyHandlers[prop])(value);
		}
		else
			throw std::exception("Unknown property name");
	}

	void SourceTASReader::HandleSettings(const std::string & value)
	{
		currentScript.AddScriptLine(new PropertyLine(origLine, value));
	}

	void SourceTASReader::ParseVariables()
	{
		while (ParseLine())
		{
			if (IsFramesLine())
			{
				break;
			}

			if (!freezeVariables)
				ParseVariable();
		}

		variables.Iteration(searchType);
		variables.PrintState();
	}

	void SourceTASReader::ParseVariable()
	{
		if (isLineEmpty())
			return;

		std::string type;
		std::string name;
		std::string value;
		GetTriplet(lineStream, type, name, value, ' ');
		variables.AddNewVariable(type, name, value);
	}

	void SourceTASReader::ParseFrames()
	{
		while (ParseLine())
		{
			ParseFrameBulk();
		}
	}

	void SourceTASReader::ParseFrameBulk()
	{
		if (isLineEmpty())
		{
			currentScript.AddScriptLine(new ScriptLine(origLine));
			return;
		}	
		else if (editedLine.find("ss") == 0)
		{
			currentScript.AddScriptLine(new SaveStateLine(origLine));
		}
		else
		{
			FrameBulkData info(editedLine);
			auto output = HandleFrameBulk(info);
			currentScript.AddScriptLine(new FrameLine(origLine, output));
		}
	}

	void SourceTASReader::InitPropertyHandlers()
	{
		propertyHandlers["save"] = &SourceTASReader::HandleSave;
		propertyHandlers["demo"] = &SourceTASReader::HandleDemo;
		propertyHandlers["demodelay"] = &SourceTASReader::HandleDemoDelay;
		propertyHandlers["search"] = &SourceTASReader::HandleSearch;
		propertyHandlers["playspeed"] = &SourceTASReader::HandlePlaybackSpeed;
		propertyHandlers["settings"] = &SourceTASReader::HandleSettings;

		// Conditions for automated searching
		propertyHandlers["tick"] = &SourceTASReader::HandleTickRange;
		propertyHandlers["tickend"] = &SourceTASReader::HandleTicksFromEndRange;
		propertyHandlers["posx"] = &SourceTASReader::HandleXPos;
		propertyHandlers["posy"] = &SourceTASReader::HandleYPos;
		propertyHandlers["posz"] = &SourceTASReader::HandleZPos;
		propertyHandlers["velx"] = &SourceTASReader::HandleXVel;
		propertyHandlers["vely"] = &SourceTASReader::HandleYVel;
		propertyHandlers["velz"] = &SourceTASReader::HandleZVel;
		propertyHandlers["vel2d"] = &SourceTASReader::Handle2DVel;
		propertyHandlers["velabs"] = &SourceTASReader::HandleAbsVel;
	}

	void SourceTASReader::HandleSave(const std::string& value)
	{
		currentScript.AddScriptLine(new ScriptLine(origLine));
		currentScript.SetSave(value);
	}

	void SourceTASReader::HandleDemo(const std::string& value)
	{
		currentScript.AddScriptLine(new ScriptLine(origLine));
		demoName = value;
	}

	void SourceTASReader::HandleDemoDelay(const std::string& value)
	{
		currentScript.AddScriptLine(new ScriptLine(origLine));
		demoDelay = ParseValue<int>(value);
	}

	void SourceTASReader::HandleSearch(const std::string& value)
	{
		if (value == "low")
			searchType = SearchType::Lowest;
		else if (value == "high")
			searchType = SearchType::Highest;
		else if (value == "random")
			searchType = SearchType::Random;
		else
			throw std::exception("Search type was invalid");
	}

	void SourceTASReader::HandlePlaybackSpeed(const std::string & value)
	{
		currentScript.AddScriptLine(new ScriptLine(origLine));
		playbackSpeed = ParseValue<float>(value);
		if (playbackSpeed <= 0.0f)
			throw std::exception("Playback speed has to be positive");
	}

	void SourceTASReader::HandleTickRange(const std::string & value)
	{
		currentScript.AddScriptLine(new ScriptLine(origLine));
		int min, max;
		GetDoublet(value, min, max, '|');
		conditions.push_back(std::unique_ptr<Condition>(new TickRangeCondition(min, max, false)));
	}

	void SourceTASReader::HandleTicksFromEndRange(const std::string & value)
	{
		currentScript.AddScriptLine(new ScriptLine(origLine));
		int min, max;
		GetDoublet(value, min, max, '|');
		conditions.push_back(std::unique_ptr<Condition>(new TickRangeCondition(min, max, true)));
	}

	void SourceTASReader::HandlePosVel(const std::string & value, Axis axis, bool isPos)
	{
		currentScript.AddScriptLine(new ScriptLine(origLine));
		float min, max;
		GetDoublet(value, min, max, '|');
		conditions.push_back(std::unique_ptr<Condition>(new PosSpeedCondition(min, max, axis, isPos)));
	}

	bool SourceTASReader::isLineEmpty()
	{
		return editedLine.find_first_not_of(' ') == std::string::npos;
	}

	bool SourceTASReader::IsFramesLine()
	{
		return editedLine.find("frames") == 0;
	}

	bool SourceTASReader::IsVarsLine()
	{
		return editedLine.find("vars") == 0;
	}

	std::string GetVarIdentifier(std::string name)
	{
		std::ostringstream os;
		os << "[" << name << "]";

		return os.str();
	}
}
