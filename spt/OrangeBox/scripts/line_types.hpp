#pragma once
#include "parsed_script.hpp"

namespace scripts
{
	class FrameLine : public ScriptLine
	{
	public:
		FrameLine(const std::string& line, int lineNo, FrameBulkData data);
		int TickCountAdvanced() const override;
		std::string DuringLoadCmd() const override;
		const FrameBulkData& GetData() const;
		void AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const override;
	private:
		FrameBulkData data;
	};

	class PropertyLine : public ScriptLine
	{
	public:
		PropertyLine(const std::string& line, int lineNo, const std::string& duringLoadCmd = std::string(), const std::string& afterLoadCmd = std::string(), const std::string& loadSaveCmd = std::string());
		std::string DuringLoadCmd() const override;
		std::string LoadSaveCmd() const override;
		void AddAfterFrames(std::vector<afterframes_entry_t>& entries, int runningTick) const override;
	private:
		std::string duringLoadCmd;
		std::string afterLoadCmd;
		std::string loadSaveCmd;
	};

	class SaveStateLine : public ScriptLine
	{
	public:
		SaveStateLine(const std::string& line, int lineNo);
	};
}