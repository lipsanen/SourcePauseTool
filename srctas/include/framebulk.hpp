#pragma once

#include <functional>
#include <string>

namespace srctas
{
	struct FrameBulk;
	
	struct AimYawState
	{
		AimYawState() { Yaw = 0; Auto = true; }

		double Yaw;
		bool Auto;
	};

	struct ErrorReturn
	{
		ErrorReturn() = default;
		ErrorReturn(bool value);
		ErrorReturn(std::string value, bool bvalue);

		std::string Error;
		bool Success;
	};

	struct FrameBulk;
	typedef ErrorReturn(*BulkFunc)(FrameBulk& bulk, std::size_t& lineIndex, const std::string& line);

	struct IndexHandler
	{
		BulkFunc handler;
		int Field;
	};

	struct MovementState
	{
		bool Strafe;
		int StrafeType;
		int JumpType;
		bool Lgagst;
		bool AutoJump;
		bool Duckspam;
		bool Jumpbug;
		bool StrafeYawSet;
		double StrafeYaw;
	};

	struct FrameBulk
	{
		MovementState Movement;

		// Field 2 - Regular movement
		bool Forward;
		bool Left;
		bool Right;
		bool Back;
		bool Up;
		bool Down;

		// Field 3 - Misc
		bool Jump;
		bool Duck;
		bool Use;
		bool Attack1;
		bool Attack2;
		bool Reload;
		bool Walk;
		bool Sprint;

		// Field 4 - Aiming
		bool AimSet;
		double AimPitch;
		AimYawState AimYaw;
		int AimFrames;
		int Cone;

		// Field 6 - Frames
		int Frames;

		// Field 7 - Commands
		std::string Commands;

		FrameBulk();
		std::string GenerateString();
		static ErrorReturn ParseFrameBulk(const std::string& line, FrameBulk& bulk);
	};
}