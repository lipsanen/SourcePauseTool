#include "stdafx.h"
#include "parsed_script2.hpp"
#include "..\spt-serverplugin.hpp"
#include "file.hpp"
#include "..\cvars.hpp"
#include "..\features\demo.hpp"
#include "framebulk_handler2.hpp"
#include "thirdparty\md5.hpp"

namespace scripts2
{
	void ParsedScript::AddFrameBulk(FrameBulkOutput& output)
	{
		if (output.ticks >= 0)
		{
			AddAfterFramesEntry(afterFramesTick, output.initialCommand);
			AddAfterFramesEntry(afterFramesTick, output.repeatingCommand);
			afterFramesTick += output.ticks;
		}
		else if (output.ticks == NO_AFTERFRAMES_BULK)
		{
			AddAfterFramesEntry(NO_AFTERFRAMES_BULK, output.initialCommand);
		}
		else
		{
			throw std::exception("Frame bulk length was negative");
		}
	}

	void ParsedScript::Reset()
	{
		scriptName.clear();
		afterFramesTick = 0;
		afterFramesEntries.clear();
	}

	void ParsedScript::Init(std::string name)
	{
		this->scriptName = std::move(name);
		int saveStateIndex = -1;
	}

	void ParsedScript::AddAfterFramesEntry(long long int tick, std::string command)
	{
		afterFramesEntries.push_back(afterframes_entry_t(tick, std::move(command)));
	}

	Savestate::Savestate(int tick, int index, std::string key) : tick(tick), index(index), key(std::move(key))
	{
		TestExists();
	}

	void Savestate::TestExists()
	{
		exists = FileExists(GetGameDir() + "\\SAVE\\" + key + ".sav");
	}
} // namespace scripts
