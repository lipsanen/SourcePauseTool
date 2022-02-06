#pragma once

// remove buttons and only leave vectorial strafing in
// remove strafe dir and hull stuff
// have some generic interface for all movement
// add the strafe settings so that they can be set as a standalone framebulk


namespace srctas
{
	struct Vector
	{
		Vector(float x, float y, float z) : x(x), y(y), z(z) {}

		float x, y, z;

		float& operator[](int i)
		{
			if (i == 0)
				return x;
			else if (i == 1)
				return y;
			else
				return z;
		}

		float operator[](int i) const
		{
			if (i == 0)
				return x;
			else if (i == 1)
				return y;
			else
				return z;
		}
	};

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
	};
	
	struct PlayerState
	{
		Vector Velocity;
		Vector Viewangles;
		PositionType posType;
		bool Ducking;
		bool CanJumpbug;
		bool CantJump;
		bool ReduceWishspeed;
	};

	struct StrafeSettings
	{
		double LgagstMinSpeed;
		double LgagstMaxSpeed;
		double CappedSpeed;
		double AFHLength;
		bool UseAFH;
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

	struct ProcessedFrame
	{
		bool Processed; // Should apply strafing in ClientDLL?
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

	struct CurrentState
	{
		float LgagstMinSpeed;
		bool LgagstFullMaxspeed;
	};

	enum class StrafeType
	{
		MAXACCEL = 0,
		MAXANGLE = 1,
		CAPPED = 2,
		DIRECTION = 3
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

	double MaxAccelTheta(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed);

	double MaxAccelIntoYawTheta(const PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		double vel_yaw,
		double yaw);

	double MaxAngleTheta(const PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		bool& safeguard_yaw);

	void VectorFME(PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		const Vector& a);

	double ButtonsPhi(Button button);

	Button GetBestButtons(double theta, bool right);

	void SideStrafeGeneral(const PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		const StrafeButtons& strafeButtons,
		bool useGivenButtons,
		Button& usedButton,
		double vel_yaw,
		double theta,
		bool right,
		Vector& velocity,
		double& yaw);

	double YawStrafeMaxAccel(PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		const StrafeButtons& strafeButtons,
		bool useGivenButtons,
		Button& usedButton,
		double vel_yaw,
		double yaw);

	double YawStrafeMaxAngle(PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		const StrafeButtons& strafeButtons,
		bool useGivenButtons,
		Button& usedButton,
		double vel_yaw,
		double yaw);

	void StrafeVectorial(PlayerData& player,
		const MovementVars& vars,
		bool jumped,
		StrafeType type,
		StrafeDir dir,
		double target_yaw,
		double vel_yaw,
		ProcessedFrame& out,
		bool lockCamera);

	bool Strafe(PlayerData& player,
		const MovementVars& vars,
		bool jumped,
		StrafeType type,
		StrafeDir dir,
		double target_yaw,
		double vel_yaw,
		ProcessedFrame& out,
		const StrafeButtons& strafeButtons,
		bool useGivenButtons);

	void Friction(PlayerData& player, bool onground, const MovementVars& vars);

	bool LgagstJump(PlayerData& player, const MovementVars& vars);
} // namespace Strafe
