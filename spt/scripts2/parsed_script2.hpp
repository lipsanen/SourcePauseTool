#pragma once

#include <string>
#include <vector>
#include "..\features\afterframes.hpp"

namespace scripts2
{
	struct FrameBulkOutput;

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
		std::vector<afterframes_entry_t> afterFramesEntries;

		void Reset();
		void Init(std::string name);
		void AddFrameBulk(FrameBulkOutput& output);
		void AddAfterFramesEntry(long long int tick, std::string command);
		void SetSave(std::string save);
		int GetScriptLength()
		{
			return afterFramesTick;
		}

	private:
		std::string scriptName;
		int afterFramesTick;
	};
} // namespace scripts