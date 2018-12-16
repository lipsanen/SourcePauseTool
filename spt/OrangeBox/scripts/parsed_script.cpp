#include "stdafx.h"
#include "parsed_script.hpp"
#include "..\..\utils\md5.hpp"
#include "..\spt-serverplugin.hpp"
#include "framebulk_handler.hpp"
#include "..\cvars.hpp"
#include "..\..\utils\file.hpp"
#include "line_types.hpp"

namespace scripts
{
	void ParsedScript::SetSave(const std::string& saveName)
	{
		this->saveName = saveName;
	}
	void ParsedScript::AddDuringLoadCmd(const std::string& cmd)
	{
		completeDuringLoad.push_back(';');
		completeDuringLoad += cmd;
	}

	void ParsedScript::AddInitCommand(const std::string& cmd)
	{
		completeInitCommand.push_back(';');
		completeInitCommand += cmd;
	}

	void ParsedScript::AddSaveState(int currentTick)
	{
		saveStates.push_back(GetSaveStateInfo(currentTick));
	}

	int ParsedScript::ChooseSave(int maxLength)
	{
		int saveStateIndex = -1;
		int startTick = 0;

		for (size_t i = 0; i < saveStates.size(); ++i)
		{
			if (saveStates[i].tick >= maxLength)
				break;

			if (saveStates[i].exists && tas_script_savestates.GetBool())
				saveStateIndex = i;
			else
			{
				afterFramesEntries.push_back(afterframes_entry_t(saveStates[i].tick, "save " + saveStates[i].key));
			}
		}
		
		if (saveStateIndex != -1)
		{
			startTick = saveStates[saveStateIndex].tick;
			saveName = saveStates[saveStateIndex].key;
		}

		return startTick;
	}

	ParsedScript::ParsedScript()
	{
	}

	const std::vector<afterframes_entry_t>& ParsedScript::GetAfterFramesEntries() const
	{
		return afterFramesEntries;
	}

	const std::string & ParsedScript::GetInitCommand() const
	{
		return completeInitCommand;
	}

	const std::string & ParsedScript::GetDuringLoadCmd() const
	{
		return completeDuringLoad;
	}

	void ParsedScript::Reset()
	{
		scriptName.clear();
		scriptLength = 0;
		afterFramesEntries.clear();
		completeInitCommand = "sv_cheats 1; y_spt_pause 0;_y_spt_afterframes_await_load; _y_spt_afterframes_reset_on_server_activate 0; _y_spt_resetpitchyaw";
		completeDuringLoad.clear();
		lines.clear();
		saveStates.clear();
	}

	void ParsedScript::Init(int maxLength)
	{
		ParseLines();
		int start = ChooseSave(maxLength);
		ChopCommands(start, maxLength);

		if (!saveName.empty())
			AddInitCommand("load " + saveName);
	}

	void ParsedScript::ParseLines()
	{
		int runningTick = 0;
		for (std::size_t i = 0; i < lines.size(); ++i)
		{
			auto pointer = lines[i].get();
			auto load = pointer->DuringLoadCmd();
			auto init = pointer->LoadSaveCmd();

			if (!load.empty())
				AddDuringLoadCmd(load);
			if (init.empty())
				AddInitCommand(init);

			pointer->AddAfterFrames(afterFramesEntries, runningTick);
			runningTick += pointer->TickCountAdvanced();
			
			SaveStateLine* sl = dynamic_cast<SaveStateLine*>(pointer);
			if (sl)
				AddSaveState(runningTick);
		}
		scriptLength = runningTick;
	}

	void ParsedScript::ChopCommands(int start, int end)
	{
		if (end == UNLIMITED_LENGTH)
			end = GetScriptLength();

		for (size_t i = 0; i < afterFramesEntries.size(); ++i)
		{
			if (afterFramesEntries[i].framesLeft >= start && afterFramesEntries[i].framesLeft <= end)
			{
				afterFramesEntries[i].framesLeft -= start;
			}
			else
			{
				afterFramesEntries.erase(afterFramesEntries.begin() + i);
				--i;
			}

		}
	}

	void ParsedScript::AddScriptLine(ScriptLine* line)
	{
		int prevLineNumber = 0;
		int currentLineNumber = line->LineNumber();

		if (!lines.empty())
			prevLineNumber = lines[lines.size()-1]->LineNumber();

		if (prevLineNumber == currentLineNumber)
			lines.pop_back();
		else if (prevLineNumber != currentLineNumber - 1)
		{
			try { delete line; } catch(...) { }
			throw std::exception("A line was not passed to ParsedScript.");
		}
			

		lines.push_back(std::unique_ptr<ScriptLine>(line));
	}

	void ParsedScript::WriteScriptToStream(std::ostream& stream, int length, std::string lastCmd) const
	{
		if (length == UNLIMITED_LENGTH)
			length = scriptLength;
		int currentTick = 0;
		for (int i = 0; i < lines.size(); ++i)
		{
			currentTick += lines[i]->TickCountAdvanced();
			FrameLine* fl = dynamic_cast<FrameLine*>(lines[i].get());
			if (fl)
			{
				auto& data = fl->GetData();
				std::string bulkString = data.input;
				if (currentTick >= length)
				{
					ModifyLength(bulkString, length - currentTick);
					stream << bulkString << '\n';
					break;
				}
				else
					stream << lines[i]->GetLine() << '\n';
			}
			else
				stream << lines[i]->GetLine() << '\n';
		}

		if (currentTick < length)
			stream << GenerateBulkString('-', length - currentTick, "") << '\n';

		stream << GenerateBulkString('-', 0, lastCmd) << '\n';
	}

	Savestate ParsedScript::GetSaveStateInfo(int currentTick)
	{
		std::string data = saveName;

		for (auto& entry : afterFramesEntries)
		{
			data += std::to_string(entry.framesLeft);
			data += entry.command;
		}
		
		MD5 hash(data);
		std::string digest = hash.hexdigest();

		return Savestate(currentTick, afterFramesEntries.size(), "ss-" + digest + "-" + std::to_string(currentTick));
	}

	Savestate::Savestate(int tick, int index, std::string key) : tick(tick), index(index), key(std::move(key))
	{
		TestExists();
	}

	void Savestate::TestExists()
	{
		exists = FileExists(GetGameDir() + "\\SAVE\\" + key + ".sav");
	}

	ScriptLine::ScriptLine(const std::string & line, int lineNo) : line(line), lineNo(lineNo)
	{
	}

	std::string ScriptLine::LoadSaveCmd() const
	{
		return std::string();
	}

	std::string ScriptLine::DuringLoadCmd() const
	{
		return std::string();
	}

	void ScriptLine::AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const
	{
	}

}
