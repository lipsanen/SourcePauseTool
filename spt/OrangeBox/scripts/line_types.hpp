#pragma once
#include "parsed_script.hpp"

namespace scripts
{
	extern std::string EMPTY;

	class FrameLine : public ScriptLine
	{
	public:
		FrameLine(const std::string& line, FrameBulkData data);
		int TickCountAdvanced() const override;
		const std::string& DuringLoadCmd() const override;
		void AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const override;
	private:
		FrameBulkData data;
	};

	class PropertyLine : public ScriptLine
	{
	public:
		PropertyLine(const std::string& line, const std::string& duringLoadCmd = EMPTY, const std::string& afterLoadCmd = EMPTY, const std::string& loadSaveCmd = EMPTY);
		const std::string& DuringLoadCmd() const override;
		const std::string& LoadSaveCmd() const override;
		void AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const override;
	private:
		std::string duringLoadCmd;
		std::string afterLoadCmd;
		std::string loadSaveCmd;
	};

	class SaveStateLine : public ScriptLine
	{
	public:
		SaveStateLine(const std::string& line);
	};
}