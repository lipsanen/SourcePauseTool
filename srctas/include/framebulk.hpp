#pragma once

#include <functional>
#include <string>

namespace srctas
{
	struct ErrorReturn
	{
		ErrorReturn() = default;
		ErrorReturn(bool value);
		ErrorReturn(std::string value, bool bvalue);

		std::string Error;
		bool Success;
	};

	struct _InternalAimYawState
	{
		_InternalAimYawState() { Yaw = 0; Auto = true; }

		double Yaw;
		bool Auto;
	};

	enum class StrafeType
	{
		MAXACCEL = 0,
		MAXANGLE = 1,
		CAPPED = 2,
		DIRECTION = 3
	};

	struct _InternalFramebulk
	{
		// Field 1 - Autofuncs
		bool Strafe;
		int StrafeType;
		int JumpType;
		bool Lgagst;
		bool AutoJump;
		bool Duckspam;
		bool Jumpbug;

		// Field 2 - Misc
		bool Jump;
		bool Duck;
		bool Use;
		bool Attack1;
		bool Attack2;
		bool Reload;
		bool Walk;
		bool Sprint;

		// Field 3 - Aiming
		bool AimSet;
		double AimPitch;
		_InternalAimYawState AimYaw;
		int AimFrames;
		int Cone;

		// Field 4 - Strafe yaw
		bool StrafeYawSet;
		double StrafeYaw;

		// Field 5 - Frames
		int Frames;

		// Field 6 - Commands
		std::string Commands;

		_InternalFramebulk();
		static ErrorReturn ParseFrameBulk(const std::string& line, _InternalFramebulk& bulk);
	};


	struct MovementInput
	{
		bool Strafe;

		StrafeType strafeType;
		int JumpType;
		double StrafeYaw;
		bool Lgagst;

		bool AutoJump;
		bool Duckspam;
		bool Jumpbug;

		static ErrorReturn MovementInputFromInternal(const std::string& line, MovementInput& state, _InternalFramebulk& bulk);
	};

	struct ButtonsInput
	{
		bool Jump;
		bool Duck;
		bool Use;
		bool Attack1;
		bool Attack2;
		bool Reload;
		bool Walk;
		bool Sprint;

		static ErrorReturn ButtonsInputFromInternal(const std::string& line, ButtonsInput& state, _InternalFramebulk& bulk);
	};

	struct AimState
	{
		bool AimSet;

		double AimPitch;
		double AimYaw;
		int AimFrames;
		int Cone;
		bool Auto;

		static ErrorReturn AimStateFromInternal(const std::string& line, AimState& state, _InternalFramebulk& bulk);
	};

	struct Framebulk
	{
		MovementInput movementInput;
		ButtonsInput buttonsInput;
		AimState aimState;

		int Frames;
		std::string Commands;

		// add strafe settings stuff

		Framebulk() {};
		static ErrorReturn ParseFrameBulk(const std::string& line, Framebulk& bulk);
	};
}