#pragma once

#include <functional>
#include <string>

namespace srctas
{
	struct FrameBulk;

	enum class AimSetState { Set, Unset, Reset };

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

	struct FrameBulk
	{
		// Field 1 - Autofuncs
		bool Strafe;
		int StrafeType;
		int JumpType;
		bool Lgagst;
		bool AutoJump;
		bool Duckspam;
		bool Jumpbug;

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
		AimSetState AimSet;
		double AimPitch;
		double AimYaw;
		int AimFrames;
		int Cone;

		// Field 5 - Strafe pitch
		bool StrafePitchSet;
		double StrafePitch;

		// Field 6 - Strafe yaw
		bool StrafeYawSet;
		double StrafeYaw;

		// Field 7 - Frames
		int Frames;

		// Field 8 - Commands
		std::string Commands;

		std::string GenerateString();
		static ErrorReturn ParseFrameBulk(const std::string& line, FrameBulk& bulk);
	};
}