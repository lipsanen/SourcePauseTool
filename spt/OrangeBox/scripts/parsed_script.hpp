#pragma once

#include <string>
#include <vector>
#include "..\modules\ClientDLL.hpp"
#include "framebulk_handler.hpp"

namespace scripts
{
	constexpr int UNLIMITED_LENGTH = -1;

	class ScriptLine
	{
	public:
		ScriptLine(const std::string& line, int lineNo);
		const std::string& GetLine() const { return line; }
		virtual int TickCountAdvanced() const { return 0; }
		virtual std::string LoadSaveCmd() const;
		virtual std::string DuringLoadCmd() const;
		virtual void AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const;
		int LineNumber() const { return lineNo; }
	protected:
		std::string line;
		int lineNo;
	};

	struct Savestate
	{
		Savestate() {}
		Savestate(int tick, int index, std::string key);
		int index;
		int tick;
		std::string key;
		bool exists;

	private:
		void TestExists();
	};

	class ParsedScript
	{
	public:
		ParsedScript();

		const std::vector<afterframes_entry_t>& GetAfterFramesEntries() const;
		const std::string& GetInitCommand() const;
		const std::string& GetDuringLoadCmd() const;
		
		void Reset();
		void Init(int maxTick);
		void AddScriptLine(ScriptLine* line);
		void WriteScriptToStream(std::ostream& stream, int length, std::string lastCmd = std::string()) const;
		int GetScriptLength() { return scriptLength; }
		void SetSave(const std::string& saveName);
	private:
		void AddDuringLoadCmd(const std::string& cmd);
		void AddInitCommand(const std::string& cmd);
		void AddSaveState(int currentTick);
		int ChooseSave(int maxLength);
		void ParseLines();
		void ChopCommands(int start, int end);

		std::string saveName;
		std::string scriptName;
		int scriptLength;
		Savestate GetSaveStateInfo(int currentTick);

		std::string completeInitCommand;
		std::string completeDuringLoad;
		std::vector<afterframes_entry_t> afterFramesEntries;

		std::vector<std::unique_ptr<ScriptLine>> lines;
		std::vector<Savestate> saveStates;
	};

}