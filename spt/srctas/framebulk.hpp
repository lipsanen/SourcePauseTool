#pragma once
#include <string>
#include <map>
#include <vector>
#include "cmd_parsing.hpp"

namespace srctas
{
	enum class KeyState : uint8_t
	{
		Noop = 0,
		Unpressed = 1,
		Pressed = 2
	};

	struct FrameBulkError
	{
		bool m_bError;
		int m_iIndex;
		std::string m_sMessage;
	};

	struct FrameBulkValue
	{
		union
		{
			int m_iValue;
			float m_fValue;
		};
		KeyState state;

		char IntToChar();
		std::string IntToString();
		std::string DoubleToString();
		void GetDoubleOutput(std::string& output, const char* cvar);
		void GetIntOutput(std::string& output, const char* cvar);

		static FrameBulkValue FromInt(int value);
		static FrameBulkValue FromFloat(float value);
		FrameBulkValue()
		{
			state = KeyState::Noop;
			m_fValue = 0;
		}
	};

	class FrameBulk
	{
	public:
		FrameBulk();

		FrameBulkValue m_fPitch;
		FrameBulkValue m_fYaw;
		FrameBulkValue m_fStrafeYaw;
		FrameBulkValue m_iStrafeType;
		FrameBulkValue m_iJumpType;
		FrameBulkValue m_iStrafeDir;
		int m_iTicks;

		std::string m_sCommands;

		KeyState m_kStrafe;
		KeyState m_kLgagst;
		KeyState m_kAutojump;
		KeyState m_kW;
		KeyState m_kA;
		KeyState m_kS;
		KeyState m_kD;
		KeyState m_kUp;
		KeyState m_kDown;
		KeyState m_kJump;
		KeyState m_kDuck;
		KeyState m_kUse;
		KeyState m_kAttack1;
		KeyState m_kAttack2;
		KeyState m_kReload;
		KeyState m_kWalk;
		KeyState m_kSprint;

		std::string GetFramebulkString();
		std::string GetCommand();
		static FrameBulk Parse(const std::string& line, FrameBulkError& error);
		void ApplyCommand(const char* command);

	private:
		bool _AddCvar(CommandArg& arg0, CommandArg& value);
		bool _AddToggle(CommandArg& arg0, bool toggle);
	};
} // namespace srctas
