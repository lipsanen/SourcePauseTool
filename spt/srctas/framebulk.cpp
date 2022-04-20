#include "stdafx.h"
#include "cmd_parsing.hpp"
#include "framebulk.hpp"
#include "utils.hpp"
#include <algorithm>
#include <charconv>
#include <string.h>

namespace srctas
{
	static const char NOOP = '>';
	static const char UNPRESSED = '-';
	static const char* FRAMEBULK = "sXXXlj|wasd|jdu12rws";

	struct KeyInfo
	{
		union
		{
			KeyState FrameBulk::*m_pToMember;
			FrameBulkValue FrameBulk::*m_pToValue;
		};
		const char* m_sCommand;
		int m_iIndex;
		int m_iField;
		bool m_bToggle;

		int GetIndex()
		{
			const int FIELD1_LEN = 7;
			const int FIELD2_LEN = 5;
			if (m_iField == 0)
				return m_iIndex;
			else if (m_iField == 1)
				return FIELD1_LEN + m_iIndex;
			else if (m_iField == 2)
				return FIELD1_LEN + FIELD2_LEN + m_iIndex;
			else
				return -1; // bug
		}

		KeyInfo(KeyState FrameBulk::*ptr, const char* cmd, int index, int field)
		{
			m_pToMember = ptr;
			m_sCommand = cmd;
			m_iIndex = index;
			m_iField = field;
			m_bToggle = true;
		}

		KeyInfo(FrameBulkValue FrameBulk::*ptr, const char* cmd, int index, int field)
		{
			m_pToValue = ptr;
			m_sCommand = cmd;
			m_iIndex = index;
			m_iField = field;
			m_bToggle = false;
		}

		bool HandleChar(char c, FrameBulk& bulk)
		{
			if (m_bToggle)
			{
				if (c == NOOP)
				{
					(bulk.*m_pToMember) = KeyState::Noop;
				}
				else if (c == UNPRESSED)
				{
					(bulk.*m_pToMember) = KeyState::Unpressed;
				}
				else if (c == FRAMEBULK[GetIndex()]) // Matches the character on the line
				{
					(bulk.*m_pToMember) = KeyState::Pressed;
				}
				else
				{
					return false;
				}
			}
			else
			{
				if (c == NOOP)
				{
					(bulk.*m_pToValue).state = KeyState::Noop;
				}
				else if (c == UNPRESSED)
				{
					(bulk.*m_pToValue).state = KeyState::Unpressed;
				}
				else
				{
					int number = c - '0';

					if (number >= 0 && number <= 9)
					{
						(bulk.*m_pToValue).m_iValue = number;
						(bulk.*m_pToValue).state = KeyState::Pressed;
					}
					else
					{
						return false;
					}
				}
			}

			return true;
		}
	};

	static float atof(const char* val)
	{
		const char* end = val;

		while (*end)
		{
			++end;
		}

		float value;
		std::from_chars(val, end, value);

		return value;
	}

	static void ParseFloat(const char*& ptr, FrameBulkError& error, KeyState& state, float& value)
	{
		int parsed = sscanf(ptr, "%f", &value);

		if (parsed == 1)
		{
			state = KeyState::Pressed;
		}
		else if (*ptr == NOOP)
		{
			state = KeyState::Noop;
		}
		else if (*ptr == UNPRESSED)
		{
			state = KeyState::Unpressed;
		}
		else
		{
			error.m_bError = true;
			error.m_sMessage += format("Tried to parse an invalid float \"%s\"", ptr);
			return;
		}

		while (*ptr && *ptr != '|')
		{
			++ptr;
		}

		if (*ptr == '|')
			++ptr;
	}

	static void ParseInt(const char*& ptr, FrameBulkError& error, KeyState& state, int& value)
	{
		int parsed = sscanf(ptr, "%d", &value);

		if (parsed == 1)
		{
			state = KeyState::Pressed;
		}
		else if (*ptr == NOOP)
		{
			state = KeyState::Noop;
		}
		else if (*ptr == UNPRESSED)
		{
			state = KeyState::Unpressed;
		}
		else
		{
			error.m_bError = true;
			error.m_sMessage = format("Tried to parse an int: \"%s\"", ptr);
			return;
		}

		while (*ptr && *ptr != '|')
		{
			++ptr;
		}

		if (*ptr == '|')
			++ptr;
	}

	static const char* FloatToCString(float value)
	{
		thread_local static char BUFFER[128];
		auto result = std::to_chars(BUFFER, BUFFER + sizeof(BUFFER) - 1, value);
		*result.ptr = '\0';

		return BUFFER;
	}

	struct FrameBulkValueInfo
	{
		FrameBulkValue FrameBulk::*m_pToMember;
		const char* m_sCommand;
		bool isDouble;

		static FrameBulkValueInfo CreateDouble(FrameBulkValue FrameBulk::*ptr, const char* cmd)
		{
			FrameBulkValueInfo info;
			info.isDouble = true;
			info.m_pToMember = ptr;
			info.m_sCommand = cmd;

			return info;
		}

		static FrameBulkValueInfo CreateInt(FrameBulkValue FrameBulk::*ptr, const char* cmd)
		{
			FrameBulkValueInfo info;
			info.isDouble = false;
			info.m_pToMember = ptr;
			info.m_sCommand = cmd;

			return info;
		}
	};

	struct FramebulkParserData
	{
		std::vector<KeyInfo> m_vKeyInfos;
		std::vector<FrameBulkValueInfo> m_vValueInfos;

		static FramebulkParserData* GetData()
		{
			static FramebulkParserData data;
			return &data;
		}

		FramebulkParserData()
		{
			m_vValueInfos.push_back(
			    FrameBulkValueInfo::CreateDouble(&FrameBulk::m_fPitch, "_y_spt_setpitch"));
			m_vValueInfos.push_back(FrameBulkValueInfo::CreateDouble(&FrameBulk::m_fYaw, "_y_spt_setyaw"));
			m_vValueInfos.push_back(
			    FrameBulkValueInfo::CreateDouble(&FrameBulk::m_fStrafeYaw, "tas_strafe_yaw"));

			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kStrafe, "tas_strafe", 0, 0));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_iStrafeType, "tas_strafe_type", 1, 0));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_iJumpType, "tas_strafe_jumptype", 2, 0));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_iStrafeDir, "tas_strafe_dir", 3, 0));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kLgagst, "tas_lgagst", 4, 0));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kAutojump, "tas_autojump", 5, 0));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kW, "forward", 0, 1));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kA, "moveleft", 1, 1));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kS, "back", 2, 1));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kD, "moveright", 3, 1));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kJump, "jump", 0, 2));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kDuck, "duck", 1, 2));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kUse, "use", 2, 2));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kAttack1, "attack", 3, 2));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kAttack2, "attack2", 4, 2));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kReload, "reload", 5, 2));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kWalk, "walk", 6, 2));
			m_vKeyInfos.push_back(KeyInfo(&FrameBulk::m_kSprint, "speed", 7, 2));
		}
	};

	FrameBulkValue FrameBulkValue::FromInt(int value)
	{
		FrameBulkValue rval;
		rval.m_iValue = value;
		rval.state = KeyState::Pressed;

		return rval;
	}

	FrameBulkValue FrameBulkValue::FromFloat(float value)
	{
		FrameBulkValue rval;
		rval.m_fValue = value;
		rval.state = KeyState::Pressed;

		return rval;
	}

	char FrameBulkValue::IntToChar()
	{
		if (state == KeyState::Noop)
		{
			return NOOP;
		}
		else if (state == KeyState::Unpressed)
		{
			return UNPRESSED;
		}
		else
		{
			int value = std::max(0, m_iValue);
			value = std::min(9, value);

			return '0' + value;
		}
	}

	std::string FrameBulkValue::DoubleToString()
	{
		char BUFFER[2];
		BUFFER[1] = '\0';
		if (state == KeyState::Noop)
		{
			BUFFER[0] = NOOP;
			return BUFFER;
		}
		else if (state == KeyState::Unpressed)
		{
			BUFFER[0] = UNPRESSED;
			return BUFFER;
		}
		else
		{
			return FloatToCString(m_fValue);
		}
	}

	std::string FrameBulkValue::IntToString()
	{
		char BUFFER[32];
		if (state == KeyState::Noop)
		{
			BUFFER[0] = NOOP;
			BUFFER[1] = '\0';
		}
		else if (state == KeyState::Unpressed)
		{
			BUFFER[0] = UNPRESSED;
			BUFFER[1] = '\0';
		}
		else
		{
			snprintf(BUFFER, sizeof(BUFFER), "%d", m_iValue);
		}

		return BUFFER;
	}

	void FrameBulkValue::GetDoubleOutput(std::string& output, const char* cvar)
	{
		char BUFFER[256];
		if (state != KeyState::Pressed)
		{
			return;
		}
		else
		{
			snprintf(BUFFER, sizeof(BUFFER), ";%s %s", cvar, FloatToCString(m_fValue));
			output += BUFFER;
		}
	}

	void FrameBulkValue::GetIntOutput(std::string& output, const char* cvar)
	{
		char BUFFER[256];
		if (state != KeyState::Pressed)
		{
			return;
		}
		else
		{
			snprintf(BUFFER, sizeof(BUFFER), ";%s %d", cvar, m_iValue);
			output += BUFFER;
		}
	}

	FrameBulk::FrameBulk()
	{
		m_kStrafe = KeyState::Noop;
		m_kLgagst = KeyState::Noop;
		m_kAutojump = KeyState::Noop;
		m_kW = KeyState::Noop;
		m_kA = KeyState::Noop;
		m_kS = KeyState::Noop;
		m_kD = KeyState::Noop;
		m_kUp = KeyState::Noop;
		m_kDown = KeyState::Noop;
		m_kJump = KeyState::Noop;
		m_kDuck = KeyState::Noop;
		m_kUse = KeyState::Noop;
		m_kAttack1 = KeyState::Noop;
		m_kAttack2 = KeyState::Noop;
		m_kReload = KeyState::Noop;
		m_kWalk = KeyState::Noop;
		m_kSprint = KeyState::Noop;

		m_iTicks = 1;
	}

	std::string FrameBulk::GetFramebulkString()
	{
#define HANDLE_FIELD(index, member) \
	if (member == KeyState::Noop) \
		rval[index] = NOOP; \
	else if (member == KeyState::Unpressed) \
	rval[index] = UNPRESSED

		auto parserData = FramebulkParserData::GetData();
		std::string rval;
		rval += FRAMEBULK;

		for (auto& key : parserData->m_vKeyInfos)
		{
			if (key.m_bToggle)
			{
				if (this->*(key.m_pToMember) == KeyState::Noop)
				{
					rval[key.GetIndex()] = NOOP;
				}
				else if (this->*(key.m_pToMember) == KeyState::Unpressed)
				{
					rval[key.GetIndex()] = UNPRESSED;
				}
			}
			else
			{
				if ((this->*(key.m_pToValue)).state == KeyState::Noop)
				{
					rval[key.GetIndex()] = NOOP;
				}
				else if ((this->*(key.m_pToValue)).state == KeyState::Unpressed)
				{
					rval[key.GetIndex()] = UNPRESSED;
				}
				else
				{
					rval[key.GetIndex()] = '0' + (this->*(key.m_pToValue)).m_iValue;
				}
			}
		}

		rval += '|';
		rval += m_fPitch.DoubleToString();
		rval += '|';
		rval += m_fYaw.DoubleToString();
		rval += '|';
		rval += m_fStrafeYaw.DoubleToString();
		rval += '|';
		rval += std::to_string(m_iTicks);
		rval += '|';

		rval += m_sCommands;

		return rval;
	}

	std::string FrameBulk::GetCommand()
	{
		std::string value;
		auto parserData = FramebulkParserData::GetData();

		for (auto& key : parserData->m_vKeyInfos)
		{
			if (!key.m_bToggle)
				continue;
			if (this->*(key.m_pToMember) == KeyState::Pressed)
			{
				value += ";+";
				value += key.m_sCommand;
			}
			else if (this->*(key.m_pToMember) == KeyState::Unpressed)
			{
				value += ";-";
				value += key.m_sCommand;
			}
		}

		m_iStrafeType.GetIntOutput(value, "tas_strafe_type");
		m_iJumpType.GetIntOutput(value, "tas_strafe_jumptype");
		m_iStrafeDir.GetIntOutput(value, "tas_strafe_dir");
		m_fPitch.GetDoubleOutput(value, "_y_spt_setpitch");
		m_fYaw.GetDoubleOutput(value, "_y_spt_setyaw");
		m_fStrafeYaw.GetDoubleOutput(value, "tas_strafe_yaw");

		if (!m_sCommands.empty())
		{
			value.push_back(';');
			value += m_sCommands;
		}

		return value;
	}

	FrameBulk FrameBulk::Parse(const std::string& line, FrameBulkError& error)
	{
		auto parserData = FramebulkParserData::GetData();
		FrameBulk bulk;
		error.m_bError = false;
		error.m_sMessage.clear();
		error.m_iIndex = 0;

		int index = 0;
		int field = 0;
		int fieldIndex = 0;
		auto keyInfoIt = parserData->m_vKeyInfos.begin();
		auto keyInfoEnd = parserData->m_vKeyInfos.end();

		// Parse first 3 fields
		while (field < 3 && static_cast<std::size_t>(index) < line.size())
		{
			char c = line[index];
			if (c == '|')
			{
				fieldIndex = 0;
				++field;

				while (keyInfoIt != keyInfoEnd && keyInfoIt->m_iField < field)
					++keyInfoIt;
			}
			else if (keyInfoIt != keyInfoEnd && keyInfoIt->m_iIndex == fieldIndex
			         && keyInfoIt->m_iField == field)
			{
				if (!keyInfoIt->HandleChar(c, bulk))
				{
					error.m_bError = true;
					error.m_sMessage = format("Invalid character %c", c);
					error.m_iIndex = index;

					return bulk;
				}

				++keyInfoIt;
				++fieldIndex;
			}
			++index;
		}

		if (field != 3)
		{
			error.m_iIndex = index;
			error.m_bError = true;
			error.m_sMessage = "Framebulk does not contain all required fields.";
			return bulk;
		}

#define REPORT_ERROR_FRAMEBULK() \
	if (error.m_bError) \
	{ \
		error.m_iIndex = ptr - line.c_str(); \
		return bulk; \
	}

		const char* ptr = line.c_str() + index;

		ParseFloat(ptr, error, bulk.m_fPitch.state, bulk.m_fPitch.m_fValue);
		REPORT_ERROR_FRAMEBULK();
		ParseFloat(ptr, error, bulk.m_fYaw.state, bulk.m_fYaw.m_fValue);
		REPORT_ERROR_FRAMEBULK();
		ParseFloat(ptr, error, bulk.m_fStrafeYaw.state, bulk.m_fStrafeYaw.m_fValue);
		REPORT_ERROR_FRAMEBULK();
		KeyState temp;
		ParseInt(ptr, error, temp, bulk.m_iTicks);
		REPORT_ERROR_FRAMEBULK();

		bulk.m_sCommands += ptr;

		return bulk;
	}

	static const char* SkipToNextCmd(const char*& command)
	{
		bool seenSemicolon = false;
		const char* end = command;
		while (*command && (*command == ';' || std::isspace(*command) || !seenSemicolon))
		{
			if (!seenSemicolon)
			{
				end = command;
			}

			if (*command == ';')
			{
				seenSemicolon = true;
			}
			++command;
		}

		// If we didnt see a semicolon, return the end of the string
		if (!seenSemicolon)
		{
			return command;
		}
		else
		{
			return end;
		}
	}

	void FrameBulk::ApplyCommand(const char* command)
	{
		if (command == nullptr)
			return;

		const char* orig;

		while (*command)
		{
			orig = command;
			CommandArg arg0 = CommandArg::ParseArg(command);

			if (*arg0.commandString == '+' || *arg0.commandString == '-')
			{
				bool toggle = *arg0.commandString == '+';
				if (_AddToggle(arg0, toggle))
				{
					SkipToNextCmd(command);
					continue;
				}
			}

			CommandArg arg1 = CommandArg::ParseArg(command);

			// Parse second arg
			if (arg1.commandString[0])
			{
				// Handle special convars
				if (_AddCvar(arg0, arg1))
				{
					SkipToNextCmd(command);
					continue;
				}
			}

			const char* end = SkipToNextCmd(command);
			std::size_t size = end - orig;

			if (!m_sCommands.empty())
				m_sCommands.push_back(';');
			m_sCommands.append(orig, size);
		}
	}

	bool FrameBulk::_AddCvar(CommandArg& arg0, CommandArg& value)
	{
		auto parserData = FramebulkParserData::GetData();

		for (auto& key : parserData->m_vKeyInfos)
		{
			if (!key.m_bToggle && strcmp(arg0.commandString, key.m_sCommand) == 0)
			{
				(this->*(key.m_pToValue)).state = KeyState::Pressed;
				// These are single letter fields, so the value has to be between 0 and 9
				int iValue = std::atoi(value.commandString);
				iValue = std::max(0, iValue);
				iValue = std::min(9, iValue);

				(this->*(key.m_pToValue)).m_iValue = iValue;
				return true;
			}
		}

		for (auto& key : parserData->m_vValueInfos)
		{
			if (strcmp(arg0.commandString, key.m_sCommand) == 0)
			{
				(this->*(key.m_pToMember)).state = KeyState::Pressed;
				if (key.isDouble)
				{
					(this->*(key.m_pToMember)).m_fValue = atof(value.commandString);
				}
				else
				{
					(this->*(key.m_pToMember)).m_iValue = std::atoi(value.commandString);
				}

				return true;
			}
		}

		return false;
	}

	bool FrameBulk::_AddToggle(CommandArg& arg0, bool toggle)
	{
		auto parserData = FramebulkParserData::GetData();

		for (auto& key : parserData->m_vKeyInfos)
		{
			if (key.m_bToggle && strcmp(arg0.commandString + 1, key.m_sCommand) == 0)
			{
				(this->*(key.m_pToMember)) = toggle ? KeyState::Pressed : KeyState::Unpressed;
				return true;
			}
		}

		// For any non matching toggles, take the first argument and add it to the command field at the end
		// Taking the first argument gets rid of potential keycodes
		if (!m_sCommands.empty())
		{
			// Remove any matching opposite toggle
			arg0.commandString[0] = toggle ? '-' : '+';                 // Invert the toggle
			auto ptr = strstr(m_sCommands.c_str(), arg0.commandString); // Search for the opposite toggle

			if (ptr != nullptr)
			{
				// Found, remove it from the string
				m_sCommands.erase(ptr - m_sCommands.c_str(), strlen(arg0.commandString));
				// If we find semicolon at where the command was, remove it
				if (!m_sCommands.empty() && m_sCommands[ptr - m_sCommands.c_str()] == ';')
				{
					m_sCommands.erase(ptr - m_sCommands.c_str(), 1);
				}
			}

			arg0.commandString[0] = toggle ? '+' : '-';
			if (!m_sCommands.empty())
			{
				m_sCommands.push_back(';');
			}
		}
		m_sCommands += arg0.commandString;

		return true;
	}
} // namespace srctas