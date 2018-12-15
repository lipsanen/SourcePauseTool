#pragma once

#include "srctas_reader.hpp"

namespace scripts
{
	const int FLOAT_PRECISION = 10;
	const float INVALID_ANGLE = -360;
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
		void AddForcedCommand(const std::string& cmd);
		void AddCommand(const std::string& cmd, const std::pair<int, int>& field);
		void AddForcedPlusMinusCmd(const std::string& command, bool set);
		void AddPlusMinusCmd(const std::string& command, bool set, const std::pair<int, int>& field);
		bool NoopField(const std::pair<int, int>& field);
		void ValidateFieldFlags(FrameBulkInfo& frameBulkInfo, const std::string& fields, int index);
		bool ContainsFlag(const std::pair<int, int>& key, const std::string& flag);
		FrameBulkOutput data;
	private:
		std::map<std::pair<int, int>, std::string> dataMap;
	};

	void ModifyLength(std::string& bulk, int addition);
	bool AngleInvalid(float angle);
	std::string GenerateBulkString(char fillChar, int ticks, const std::string& cmd, float yaw = INVALID_ANGLE, float pitch = INVALID_ANGLE);
	FrameBulkOutput GetNoopBulk(int length, const std::string& cmd);
	FrameBulkOutput GetEmptyBulk(int length, const std::string& cmd);
	FrameBulkOutput HandleFrameBulk(FrameBulkInfo& frameBulkInfo);
}
