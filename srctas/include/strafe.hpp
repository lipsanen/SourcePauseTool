#pragma once

#include "vector.hpp"
#include "framebulk.hpp"
// remove buttons and only leave vectorial strafing in
// remove strafe dir and hull stuff
// have some generic interface for all movement
// add the strafe settings so that they can be set as a standalone framebulk


namespace srctas
{
	enum class PositionType
	{
		GROUND = 0,
		AIR,
		WATER
	};

	struct GameSettings
	{
		float Accelerate;
		float Airaccelerate;
		float Frametime;
		float Friction;
		float Maxspeed;
		float Stopspeed;
		float WishspeedCap;

		float EntGravity;
		float Maxvelocity;
		float Gravity;
		float Stepsize;
		float Bounce;
	};
	
	struct PlayerState
	{
		Vector Velocity;
		Vector Viewangles;
		PositionType posType;
		float EntFriction;
		bool Ducking;
		bool CanJumpbug;
		bool ReduceWishspeed;
	};

	struct StrafeButtons
	{
		StrafeButtons()
			: AirLeft(Button::FORWARD)
			, AirRight(Button::FORWARD)
			, GroundLeft(Button::FORWARD)
			, GroundRight(Button::FORWARD)
		{
		}

		Button AirLeft;
		Button AirRight;
		Button GroundLeft;
		Button GroundRight;
	};

	struct StrafeSettings
	{
		double LgagstMinSpeed;
		double LgagstMaxSpeed;
		double CappedSpeed;
		double AFHLength;
		bool UseAFH;
		StrafeButtons Buttons;
		bool UseGivenButtons;
	};

	// deprecated
	struct MovementVars
	{
		float Accelerate;
		float Airaccelerate;
		float EntFriction;
		float Frametime;
		float Friction;
		float Maxspeed;
		float Stopspeed;
		float WishspeedCap;

		float EntGravity;
		float Maxvelocity;
		float Gravity;
		float Stepsize;
		float Bounce;

		bool OnGround;
		bool CantJump;
		bool ReduceWishspeed;
	};

	// deprecated
	struct PlayerData
	{
		Vector UnduckedOrigin;
		Vector Velocity;
		bool Ducking;
		bool DuckPressed;
	};

	enum class Button : unsigned char
	{
		FORWARD = 0,
		FORWARD_LEFT,
		LEFT,
		BACK_LEFT,
		BACK,
		BACK_RIGHT,
		RIGHT,
		FORWARD_RIGHT
	};

	struct ProcessedFrame
	{
		bool Processed;
		bool Forward;
		bool Back;
		bool Right;
		bool Left;
		bool Jump;
		bool ForceUnduck;

		double Yaw;
		float ForwardSpeed;
		float SideSpeed;

		ProcessedFrame()
			: Processed(false)
			, Forward(false)
			, Back(false)
			, Right(false)
			, Left(false)
			, Jump(false)
			, Yaw(0)
			, ForwardSpeed(0)
			, SideSpeed(0)
			, ForceUnduck(false)
		{
		}
	};

	enum class StrafeDir
	{
		LEFT = 0,
		RIGHT = 1,
		YAW = 3
	};

	enum class HullType : int
	{
		NORMAL = 0,
		DUCKED = 1,
		POINT = 2
	};

	// Convert both arguments to doubles.
	double Atan2(double a, double b);

	double MaxAccelTheta(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings);

	double MaxAccelIntoYawTheta(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings);

	double MaxAngleTheta(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings);

	double ButtonsPhi(Button button);

	Button GetBestButtons(double theta, bool right);

	void SideStrafeGeneral(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings);

	double YawStrafeMaxAccel(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings);

	double YawStrafeMaxAngle(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings);

	void StrafeVectorial(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings);

	void StrafeOld(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings);

	ProcessedFrame Strafe(const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings);

	void Friction(PlayerData& player, bool onground, const MovementVars& vars);

	bool LgagstJump(PlayerData& player, const MovementVars& vars);
} // namespace Strafe
