#pragma once

#include <string>
#include <vector>
#include "..\modules\ClientDLL.hpp"

namespace scripts
{
	struct FrameBulkOutput;

	constexpr int UNLIMITED_LENGTH = -1;

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
		ParsedScript(int maxLength = UNLIMITED_LENGTH);
		std::string initCommand;
		std::string duringLoad;
		std::vector<afterframes_entry_t> afterFramesEntries;
		std::vector<int> saveStateIndexes;
		
		void Reset();
		void Init(std::string name);
		bool IsUnlimited();

		void AddDuringLoadCmd(const std::string& cmd);
		void AddInitCommand(const std::string& cmd);
		void AddFrameBulk(FrameBulkOutput& output);
		void AddSaveState();
		void AddAfterFramesEntry(long long int tick, std::string command);
		void SetSave(std::string save) { saveName = save; };
		int GetScriptLength() { return afterFramesTick; }
		void SetScriptMaxLength(int length) { maxLength = length; }
		bool ScriptRanOver() { return afterFramesTick > maxLength && maxLength != UNLIMITED_LENGTH; }
		int RunOverAmount() { if (ScriptRanOver()) return afterFramesTick - maxLength; else return 0; }

	private:
		int maxLength;
		std::string saveName;
		std::string scriptName;
		int afterFramesTick;
		Savestate GetSaveStateInfo();
		std::vector<Savestate> saveStates;
	};
}