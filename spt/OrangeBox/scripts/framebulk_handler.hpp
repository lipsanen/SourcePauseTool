#pragma once

#include "srctas_reader.hpp"

namespace scripts
{
	const int NO_AFTERFRAMES_BULK = -1;

	class FrameBulkOutput
	{
	private:
		std::string initialCommand;
		std::string repeatingCommand;

	public:
		const std::string& getInitialCommand() { return initialCommand; }
		const std::string& getRepeatingCommand() { return repeatingCommand; }
		void AddCommand(const std::string& newCmd);
		void AddCommand(char initChar, const std::string& newCmd);
		void AddRepeatingCommand(const std::string& newCmd);
		int ticks;
	};

	class FrameBulkInfo
	{
	public:
		FrameBulkInfo(std::istringstream& stream);
		const std::string& operator[](std::pair<int, int> i);
		bool IsInt(std::pair<int, int> i);
		bool IsFloat(std::pair<int, int> i);
		void AddCommand(const std::string& cmd, const std::pair<int, int>& field);
		void AddPlusMinusCmd(const std::string& command, bool set, const std::pair<int, int>& field);
		bool NoopField(const std::pair<int, int>& field);
		void ValidateFieldFlags(FrameBulkInfo& frameBulkInfo, const std::string& fields, int index);
		bool ContainsFlag(const std::pair<int, int>& key, const std::string& flag);
		FrameBulkOutput data;
	private:
		std::map<std::pair<int, int>, std::string> dataMap;
	};

	FrameBulkOutput HandleFrameBulk(FrameBulkInfo& frameBulkInfo);
}
