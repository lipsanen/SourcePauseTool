#include "stdafx.h"
#include "line_types.hpp"


namespace scripts
{
	std::string EMPTY;

	SaveStateLine::SaveStateLine(const std::string & line) : ScriptLine(line)
	{
	}

	PropertyLine::PropertyLine(const std::string & line, const std::string & duringLoadCmd, const std::string & afterLoadCmd, const std::string & loadSaveCmd) :
		ScriptLine(line), duringLoadCmd(duringLoadCmd), afterLoadCmd(afterLoadCmd), loadSaveCmd(loadSaveCmd)
	{
	}

	const std::string & PropertyLine::DuringLoadCmd() const
	{
		return afterLoadCmd;
	}

	void PropertyLine::AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const
	{
		entries.push_back(afterframes_entry_t(0, afterLoadCmd));
	}

	FrameLine::FrameLine(const std::string & line, FrameBulkData data) : ScriptLine(line), data(data)
	{
	}

	int FrameLine::TickCountAdvanced() const
	{
		if (data.outputData.ticks != NO_AFTERFRAMES_BULK)
			return data.outputData.ticks;
		else
			return 0;
	}

	const std::string & FrameLine::DuringLoadCmd() const
	{
		if (data.outputData.ticks == NO_AFTERFRAMES_BULK)
			return data.outputData.getInitialCommand() + ";" + data.outputData.getRepeatingCommand();
		else
			return EMPTY;
	}

	void FrameLine::AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const
	{
		if (data.outputData.ticks >= 0)
		{
			Msg("pushing afterframes %s\n", data.outputData.getInitialCommand().c_str());
			Msg("pushing afterframes %s\n", data.outputData.getRepeatingCommand().c_str());
			entries.push_back(afterframes_entry_t(runningTick, data.outputData.getInitialCommand()));
			entries.push_back(afterframes_entry_t(runningTick, data.outputData.getRepeatingCommand()));

			for (int i = 1; i < data.outputData.ticks; ++i)
				entries.push_back(afterframes_entry_t(runningTick + i, data.outputData.getRepeatingCommand()));

		}
		else if (data.outputData.ticks < -1)
			throw std::exception("Tick was negative");
	}

}
