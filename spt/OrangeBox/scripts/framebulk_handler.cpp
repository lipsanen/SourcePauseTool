#include "stdafx.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include "tier0\commonmacros.h"
#include "framebulk_handler.hpp"
#include "..\..\utils\string_parsing.hpp"
#include "dbg.h"
#include <regex>

namespace scripts
{
	typedef void(*CommandCallback) (FrameBulkData& frameBulkInfo);
	std::vector<CommandCallback> frameBulkHandlers;

	const std::string FIELD0_FILLED = "s**ljdbcgu";
	const std::string FIELD1_FILLED = "flrbud";
	const std::string FIELD2_FILLED = "jdu12rws";
	const std::string EMPTY_FIELD = "-";
	const std::string NOOP_FIELD = ">";
	const char WILDCARD = '*';
	const char DELIMITER = '|';

	const auto STRAFE = std::pair<int, int>(0, 0);
	const auto STRAFE_TYPE = std::pair<int, int>(0, 1);
	const auto JUMP_TYPE = std::pair<int, int>(0, 2);
	const auto LGAGST = std::pair<int, int>(0, 3);
	const auto AUTOJUMP = std::pair<int, int>(0, 4);
	const auto DUCKSPAM = std::pair<int, int>(0, 5);
	const auto JUMPBUG = std::pair<int, int>(0, 6);
	const auto DUCK_BEFORE_COLLISION = std::pair<int, int>(0, 7);
	const auto DUCK_BEFORE_GROUND = std::pair<int, int>(0, 8);
	const auto USE_SPAM = std::pair<int, int>(0, 9);

	const auto FORWARD = std::pair<int, int>(1, 0);
	const auto LEFT = std::pair<int, int>(1, 1);
	const auto RIGHT = std::pair<int, int>(1, 2);
	const auto BACK = std::pair<int, int>(1, 3);
	const auto UP = std::pair<int, int>(1, 4);
	const auto DOWN = std::pair<int, int>(1, 5);

	const auto JUMP = std::pair<int, int>(2, 0);
	const auto DUCK = std::pair<int, int>(2, 1);
	const auto USE = std::pair<int, int>(2, 2);
	const auto ATTACK1 = std::pair<int, int>(2, 3);
	const auto ATTACK2 = std::pair<int, int>(2, 4);
	const auto RELOAD = std::pair<int, int>(2, 5);
	const auto WALK = std::pair<int, int>(2, 6);
	const auto SPEED = std::pair<int, int>(2, 7);

	const auto YAW_KEY = std::pair<int, int>(3, 0);
	const auto PITCH_KEY = std::pair<int, int>(4, 0);
	const auto TICKS = std::pair<int, int>(5, 0);
	const auto COMMANDS = std::pair<int, int>(6, 0);

	void Field1(FrameBulkData& frameBulkInfo)
	{
		if (frameBulkInfo.ContainsFlag(STRAFE, "s"))
		{
			frameBulkInfo.AddCommand("tas_strafe 1", STRAFE);

			if (!frameBulkInfo.IsInt(JUMP_TYPE) || !frameBulkInfo.IsInt(STRAFE_TYPE))
				throw std::exception("Jump type or strafe type was not an integer");

			frameBulkInfo.AddCommand("tas_strafe_jumptype " + frameBulkInfo[JUMP_TYPE], STRAFE);
			frameBulkInfo.AddCommand("tas_strafe_type " + frameBulkInfo[STRAFE_TYPE], STRAFE);
		}
		else
			frameBulkInfo.AddCommand("tas_strafe 0", STRAFE);

		if (frameBulkInfo.ContainsFlag(AUTOJUMP, "j"))
			frameBulkInfo.AddCommand("y_spt_autojump 1", AUTOJUMP);
		else
			frameBulkInfo.AddCommand("y_spt_autojump 0", AUTOJUMP);

		frameBulkInfo.AddPlusMinusCmd("y_spt_duckspam", frameBulkInfo.ContainsFlag(DUCKSPAM, "d"), DUCKSPAM);

		// todo
#pragma warning(push)
#pragma warning(disable:4390)
		if (frameBulkInfo.ContainsFlag(USE_SPAM, "u"));
		if (frameBulkInfo.ContainsFlag(LGAGST, "l"));
		if (frameBulkInfo.ContainsFlag(JUMPBUG, "b"));
		if (frameBulkInfo.ContainsFlag(DUCK_BEFORE_COLLISION, "c"));
		if (frameBulkInfo.ContainsFlag(DUCK_BEFORE_GROUND, "g"));
#pragma warning(pop)
	}

	void Field2(FrameBulkData& frameBulkInfo)
	{
		frameBulkInfo.AddPlusMinusCmd("forward", frameBulkInfo.ContainsFlag(FORWARD,"f"), FORWARD);
		frameBulkInfo.AddPlusMinusCmd("moveleft", frameBulkInfo.ContainsFlag(LEFT,"l"), LEFT);
		frameBulkInfo.AddPlusMinusCmd("moveright", frameBulkInfo.ContainsFlag(RIGHT, "r"), RIGHT);
		frameBulkInfo.AddPlusMinusCmd("back", frameBulkInfo.ContainsFlag(BACK, "b"), BACK);
		frameBulkInfo.AddPlusMinusCmd("moveup", frameBulkInfo.ContainsFlag(UP, "u"), UP);
		frameBulkInfo.AddPlusMinusCmd("movedown", frameBulkInfo.ContainsFlag(DOWN, "d"), DOWN);
	}

	void Field3(FrameBulkData& frameBulkInfo)
	{
		frameBulkInfo.AddPlusMinusCmd("jump", frameBulkInfo.ContainsFlag(JUMP, "j") || frameBulkInfo.ContainsFlag(AUTOJUMP, "j"), JUMP);
		frameBulkInfo.AddPlusMinusCmd("duck", frameBulkInfo.ContainsFlag(DUCK, "d"), DUCK);
		frameBulkInfo.AddPlusMinusCmd("use", frameBulkInfo.ContainsFlag(USE, "u"), USE);
		frameBulkInfo.AddPlusMinusCmd("attack", frameBulkInfo.ContainsFlag(ATTACK1, "1"), ATTACK1);
		frameBulkInfo.AddPlusMinusCmd("attack2", frameBulkInfo.ContainsFlag(ATTACK2, "2"), ATTACK2);
		frameBulkInfo.AddPlusMinusCmd("reload", frameBulkInfo.ContainsFlag(RELOAD, "r"), RELOAD);
		frameBulkInfo.AddPlusMinusCmd("walk", frameBulkInfo.ContainsFlag(WALK, "w"), WALK);
		frameBulkInfo.AddPlusMinusCmd("speed", frameBulkInfo.ContainsFlag(SPEED, "s"), SPEED);
	}

	void Field4_5(FrameBulkData& frameBulkInfo)
	{
		if (frameBulkInfo.IsFloat(YAW_KEY))
		{
			if (frameBulkInfo.ContainsFlag(STRAFE, "s"))
				frameBulkInfo.AddForcedCommand("tas_strafe_yaw " + frameBulkInfo[YAW_KEY]);
			else
				frameBulkInfo.AddForcedCommand("_y_spt_setyaw " + frameBulkInfo[YAW_KEY]);
		}
		else if (frameBulkInfo[YAW_KEY] != EMPTY_FIELD)
			throw std::exception("Unable to parse the yaw angle");

		if (frameBulkInfo.IsFloat(PITCH_KEY))
			frameBulkInfo.AddForcedCommand("_y_spt_setpitch " + frameBulkInfo[PITCH_KEY]);
		else if (frameBulkInfo[PITCH_KEY] != EMPTY_FIELD)
			throw std::exception("Unable to parse the pitch angle");
	}

	void Field6(FrameBulkData& frameBulkInfo)
	{
		if (!frameBulkInfo.IsInt(TICKS))
			throw std::exception("Tick value was not an integer");

		int ticks = std::atoi(frameBulkInfo[TICKS].c_str());
		frameBulkInfo.outputData.ticks = ticks;
	}

	void Field7(FrameBulkData& frameBulkInfo)
	{
		if (!frameBulkInfo[COMMANDS].empty())
			frameBulkInfo.outputData.AddRepeatingCommand(frameBulkInfo[COMMANDS]);
	}

	void ValidateFieldFlags(FrameBulkData& frameBulkInfo)
	{
		frameBulkInfo.ValidateFieldFlags(frameBulkInfo, FIELD0_FILLED, 0);
		frameBulkInfo.ValidateFieldFlags(frameBulkInfo, FIELD1_FILLED, 1);
		frameBulkInfo.ValidateFieldFlags(frameBulkInfo, FIELD2_FILLED, 2);
	}

	void InitHandlers()
	{
		frameBulkHandlers.push_back(ValidateFieldFlags);
		frameBulkHandlers.push_back(Field1);
		frameBulkHandlers.push_back(Field2);
		frameBulkHandlers.push_back(Field3);
		frameBulkHandlers.push_back(Field4_5);
		frameBulkHandlers.push_back(Field6);
		frameBulkHandlers.push_back(Field7);
	}

	void ModifyLength(std::string& bulk, int addition)
	{
		std::istringstream stream(bulk);
		std::string line;
		bulk = "";

		for (int i = 1; std::getline(stream, line, DELIMITER); ++i)
		{
			if (i == 6)
			{
				int length = ParseValue<int>(line);
				bulk += std::to_string(addition + length);
				bulk.push_back('|');
			}
			else
			{
				bulk += line;
				if (i != 7)
					bulk.push_back('|');
			}
		}
	}

	bool AngleInvalid(float angle)
	{
		return angle == INVALID_ANGLE;
	}

	std::string GenerateBulkString(char fillChar, int ticks, const std::string& cmd, float yaw, float pitch)
	{
		std::ostringstream oss;
		
		for (int i = 0; i < FIELD0_FILLED.size(); ++i)
			oss << fillChar;
		oss << '|';
		for (int i = 0; i < FIELD1_FILLED.size(); ++i)
			oss << fillChar;
		oss << '|';
		for (int i = 0; i < FIELD2_FILLED.size(); ++i)
			oss << fillChar;
		oss << '|';
		
		oss << std::setprecision(FLOAT_PRECISION);
		if (!AngleInvalid(yaw))
			oss << yaw << '|';
		else
			oss << "-|";

		if (!AngleInvalid(pitch))
			oss << pitch << '|';
		else
			oss << "-|";

		oss << ticks << '|' << cmd;

		return oss.str();
	}

	FrameBulkData GetNoopBulk(int length, const std::string& cmd)
	{
		FrameBulkData info(GenerateBulkString('>', length, cmd));

		return HandleFrameBulk(info);
	}

	FrameBulkData GetEmptyBulk(int length, const std::string& cmd)
	{
		FrameBulkData info(GenerateBulkString('-', length, cmd));

		return HandleFrameBulk(info);
	}

	FrameBulkData HandleFrameBulk(FrameBulkData& frameBulkInfo)
	{
		if (frameBulkHandlers.empty())
			InitHandlers();

		for (auto handler : frameBulkHandlers)
			handler(frameBulkInfo);

		return frameBulkInfo;
	}

	void FrameBulkOutput::AddCommand(const std::string& newCmd)
	{
		initialCommand.push_back(';');
		initialCommand += newCmd;
	}

	void FrameBulkOutput::AddCommand(char initChar, const std::string& newCmd)
	{
		initialCommand.push_back(';');
		initialCommand.push_back(initChar);
		initialCommand += newCmd;
	}

	void FrameBulkOutput::AddRepeatingCommand(const std::string & newCmd)
	{
		repeatingCommand.push_back(';');
		repeatingCommand += newCmd;
	}

	FrameBulkData::FrameBulkData(const std::string& input) : input(input)
	{
		std::istringstream stream(input);
		int section = 0;
		std::string line;

		do
		{
			std::getline(stream, line, DELIMITER);

			// The sections after the first three are single string (could map section index to value type here but probably not worth the effort)
			if (section > 2)
			{
				dataMap[std::make_pair(section, 0)] = line;
			}
			else
			{
				for (size_t i = 0; i < line.size(); ++i)
					dataMap[std::make_pair(section, i)] = line[i];
			}

			++section;
		} while (stream.good());
	}

	const std::string& FrameBulkData::operator[](std::pair<int, int> i)
	{
		if (dataMap.find(i) == dataMap.end())
		{
			char buffer[50];
			std::snprintf(buffer, 50, "Unable to find index (%i, %i) in frame bulk", i.first, i.second);

			throw std::exception(buffer);
		}

		return dataMap[i];
	}

	bool FrameBulkData::IsInt(std::pair<int, int> i)
	{
		std::string value = this->operator[](i);
		return IsValue<int>(value);
	}

	bool FrameBulkData::IsFloat(std::pair<int, int> i)
	{
		std::string value = this->operator[](i);
		return IsValue<float>(value);
	}

	void FrameBulkData::AddForcedCommand(const std::string & cmd)
	{
		outputData.AddCommand(cmd);
	}

	void FrameBulkData::AddCommand(const std::string & cmd, const std::pair<int, int>& field)
	{
		if (NoopField(field))
			return;

		AddForcedCommand(cmd);
	}

	void FrameBulkData::AddForcedPlusMinusCmd(const std::string& command, bool set)
	{
		if (set)
			outputData.AddCommand('+', command);
		else
			outputData.AddCommand('-', command);
	}

	void FrameBulkData::AddPlusMinusCmd(const std::string & command, bool set, const std::pair<int, int>& field)
	{
		if (NoopField(field))
			return;

		AddForcedPlusMinusCmd(command, set);
	}

	bool FrameBulkData::NoopField(const std::pair<int, int>& field)
	{
		return dataMap.find(field) == dataMap.end() || this->operator[](field) == NOOP_FIELD;
	}

	void FrameBulkData::ValidateFieldFlags(FrameBulkData& frameBulkInfo, const std::string& fields, int index)
	{
		for (size_t i = 0; i < fields.length(); ++i)
		{
			auto key = std::make_pair(index, i);
			if (fields[i] != WILDCARD && frameBulkInfo.dataMap.find(key) != frameBulkInfo.dataMap.end())
			{
				auto& value = frameBulkInfo[key];
				if (value[0] != fields[i] && value[0] != '-' && value[0] != '>')
				{
					std::ostringstream os;
					os << "Expected " << fields[i] << ", got " << value << " in field: " << fields;
					throw std::exception(os.str().c_str());
				}
			}
		}
	}
	bool FrameBulkData::ContainsFlag(const std::pair<int, int>& key, const std::string & flag)
	{
		return dataMap.find(key) != dataMap.end() && this->operator[](key) == flag;
	}
}
