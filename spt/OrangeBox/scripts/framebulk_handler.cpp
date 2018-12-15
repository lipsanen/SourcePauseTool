#include "stdafx.h"
#include "framebulk_handler.hpp"
#include "..\..\utils\string_parsing.hpp"

namespace scripts
{
	typedef void(*CommandCallback) (FrameBulkInfo& frameBulkInfo);
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

	void Field1(FrameBulkInfo& frameBulkInfo)
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

	void Field2(FrameBulkInfo& frameBulkInfo)
	{
		frameBulkInfo.AddPlusMinusCmd("forward", frameBulkInfo.ContainsFlag(FORWARD,"f"), FORWARD);
		frameBulkInfo.AddPlusMinusCmd("moveleft", frameBulkInfo.ContainsFlag(LEFT,"l"), LEFT);
		frameBulkInfo.AddPlusMinusCmd("moveright", frameBulkInfo.ContainsFlag(RIGHT, "r"), RIGHT);
		frameBulkInfo.AddPlusMinusCmd("back", frameBulkInfo.ContainsFlag(BACK, "b"), BACK);
		frameBulkInfo.AddPlusMinusCmd("moveup", frameBulkInfo.ContainsFlag(UP, "u"), UP);
		frameBulkInfo.AddPlusMinusCmd("movedown", frameBulkInfo.ContainsFlag(DOWN, "d"), DOWN);
	}

	void Field3(FrameBulkInfo& frameBulkInfo)
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

	void Field4_5(FrameBulkInfo& frameBulkInfo)
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

	void Field6(FrameBulkInfo& frameBulkInfo)
	{
		if (!frameBulkInfo.IsInt(TICKS))
			throw std::exception("Tick value was not an integer");

		int ticks = std::atoi(frameBulkInfo[TICKS].c_str());
		frameBulkInfo.data.ticks = ticks;
	}

	void Field7(FrameBulkInfo& frameBulkInfo)
	{
		if (!frameBulkInfo[COMMANDS].empty())
			frameBulkInfo.data.AddRepeatingCommand(frameBulkInfo[COMMANDS]);
	}

	void ValidateFieldFlags(FrameBulkInfo& frameBulkInfo)
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
		char FIELD1[16];
		char FIELD2[16];
		char FIELD3[16];
		char FIELD4[16];
		char FIELD5[16];
		int tick;
		char FIELD6[16];
		char OUTPUT[128];
		Msg("%s comes in.\n", bulk.c_str());
		sscanf_s(bulk.c_str(), "%s|%s|%s|%s|%s|%d|%s", FIELD1, FIELD2, FIELD3, FIELD4, FIELD5, &tick, FIELD6);
		sprintf_s(OUTPUT, "%s|%s|%s|%s|%s|%d|%s\n", FIELD1, FIELD2, FIELD3, FIELD4, FIELD5, tick + addition, FIELD6);
		bulk = OUTPUT;
		Msg("%s comes out. You can't explain that.\n", bulk.c_str());
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

	FrameBulkOutput GetNoopBulk(int length, const std::string& cmd)
	{
		std::istringstream istr(GenerateBulkString('>', length, cmd));
		FrameBulkInfo info(istr);

		return HandleFrameBulk(info);
	}

	FrameBulkOutput GetEmptyBulk(int length, const std::string& cmd)
	{
		std::istringstream istr(GenerateBulkString('-', length, cmd));
		FrameBulkInfo info(istr);

		return HandleFrameBulk(info);
	}

	FrameBulkOutput HandleFrameBulk(FrameBulkInfo& frameBulkInfo)
	{
		if (frameBulkHandlers.empty())
			InitHandlers();

		for (auto handler : frameBulkHandlers)
			handler(frameBulkInfo);

		return frameBulkInfo.data;
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

	FrameBulkInfo::FrameBulkInfo(std::istringstream& stream)
	{
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

	const std::string& FrameBulkInfo::operator[](std::pair<int, int> i)
	{
		if (dataMap.find(i) == dataMap.end())
		{
			char buffer[50];
			std::snprintf(buffer, 50, "Unable to find index (%i, %i) in frame bulk", i.first, i.second);

			throw std::exception(buffer);
		}

		return dataMap[i];
	}

	bool FrameBulkInfo::IsInt(std::pair<int, int> i)
	{
		std::string value = this->operator[](i);
		return IsValue<int>(value);
	}

	bool FrameBulkInfo::IsFloat(std::pair<int, int> i)
	{
		std::string value = this->operator[](i);
		return IsValue<float>(value);
	}

	void FrameBulkInfo::AddForcedCommand(const std::string & cmd)
	{
		data.AddCommand(cmd);
	}

	void FrameBulkInfo::AddCommand(const std::string & cmd, const std::pair<int, int>& field)
	{
		if (NoopField(field))
			return;

		AddForcedCommand(cmd);
	}

	void FrameBulkInfo::AddForcedPlusMinusCmd(const std::string& command, bool set)
	{
		if (set)
			data.AddCommand('+', command);
		else
			data.AddCommand('-', command);
	}

	void FrameBulkInfo::AddPlusMinusCmd(const std::string & command, bool set, const std::pair<int, int>& field)
	{
		if (NoopField(field))
			return;

		AddForcedPlusMinusCmd(command, set);
	}

	bool FrameBulkInfo::NoopField(const std::pair<int, int>& field)
	{
		return dataMap.find(field) == dataMap.end() || this->operator[](field) == NOOP_FIELD;
	}

	void FrameBulkInfo::ValidateFieldFlags(FrameBulkInfo& frameBulkInfo, const std::string& fields, int index)
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
	bool FrameBulkInfo::ContainsFlag(const std::pair<int, int>& key, const std::string & flag)
	{
		return dataMap.find(key) != dataMap.end() && this->operator[](key) == flag;
	}
}
