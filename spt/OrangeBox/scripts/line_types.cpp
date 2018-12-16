#include "stdafx.h"
#include "line_types.hpp"


namespace scripts
{
	SaveStateLine::SaveStateLine(const std::string & line, int lineNo) : ScriptLine(line, lineNo)
	{
	}

	PropertyLine::PropertyLine(const std::string & line, int lineNo, const std::string & duringLoadCmd, const std::string & afterLoadCmd, const std::string & loadSaveCmd) :
		ScriptLine(line, lineNo), duringLoadCmd(duringLoadCmd), afterLoadCmd(afterLoadCmd), loadSaveCmd(loadSaveCmd)
	{
	}

	std::string PropertyLine::DuringLoadCmd() const
	{
		return duringLoadCmd;
	}

	std::string PropertyLine::LoadSaveCmd() const
	{
		return loadSaveCmd;
	}

	void PropertyLine::AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const
	{
		if(!afterLoadCmd.empty())
			entries.push_back(afterframes_entry_t(0, afterLoadCmd));
	}

	FrameLine::FrameLine(const std::string & line, int lineNo, FrameBulkData data) : ScriptLine(line, lineNo), data(data)
	{
	}

	int FrameLine::TickCountAdvanced() const
	{
		if (data.outputData.ticks != NO_AFTERFRAMES_BULK)
			return data.outputData.ticks;
		else
			return 0;
	}

	std::string FrameLine::DuringLoadCmd() const
	{
		if (data.outputData.ticks == NO_AFTERFRAMES_BULK)
			return data.outputData.getInitialCommand() + ";" + data.outputData.getRepeatingCommand();
		else
			return std::string();
	}

	const FrameBulkData& FrameLine::GetData() const
	{
		return data;
	}

	void FrameLine::AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const
	{
		if (data.outputData.ticks >= 0)
		{
			entries.push_back(afterframes_entry_t(runningTick, data.outputData.getInitialCommand()));
			entries.push_back(afterframes_entry_t(runningTick, data.outputData.getRepeatingCommand()));

			for (int i = 1; i < data.outputData.ticks; ++i)
				entries.push_back(afterframes_entry_t(runningTick + i, data.outputData.getRepeatingCommand()));

		}
		else if (data.outputData.ticks < -1)
			throw std::exception("Tick was negative");
	}

}
