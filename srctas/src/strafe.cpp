#include "strafe.hpp"
#include "strafe_utils.hpp"
#include <algorithm>
#include <cstddef>

namespace srctas
{
	bool OnGround(const PlayerState& playerState)
	{
		return (playerState.posType == PositionType::GROUND);
	}

	double VelTheta(const PlayerState& playerState, const MovementInput& input)
	{
		if (!IsZero<Vector, 2>(playerState.Velocity))
			return Atan2(playerState.Velocity.y, playerState.Velocity.x);
		else
			return input.StrafeYaw * M_DEG2RAD;
	}

	double Accel(double wishspeed, const PlayerState& playerState, const GameSettings& settings)
	{
		return OnGround(playerState) ? settings.Accelerate : settings.Airaccelerate;
	}

	double AccelSpeed(double accel, double wishspeed, const PlayerState& playerState, const GameSettings& settings)
	{
		return accel * wishspeed * playerState.EntFriction * settings.Frametime;
	}

	double WishspeedCap(double wishspeed, const PlayerState& playerState, const GameSettings& settings)
	{
		return OnGround(playerState) ? wishspeed : settings.WishspeedCap;
	}

	double VectorFMESpeed(double wishspeed, double theta, const PlayerState& playerState, const GameSettings& settings)
	{
		Vector avec(std::cos(theta), std::sin(theta), 0);
		float vel = Length<Vector, 2>(playerState.Velocity);
		double wishspeed_capped = WishspeedCap(wishspeed, playerState, settings);

		double tmp = wishspeed_capped - DotProduct<Vector, 2>(playerState.Velocity, avec);
		if (tmp <= 0.0)
			return;

		double accel = Accel(wishspeed, playerState, settings);
		double accelspeed = AccelSpeed(accel, wishspeed, playerState, settings);
		if (accelspeed <= tmp)
			return accelspeed;
		else
			return tmp;
	}

	double TargetTheta(double wishspeed, double targetVel, const GameSettings& settings, const PlayerState& playerState)
	{
		double accel = Accel(wishspeed, playerState, settings);
		double L = settings.WishspeedCap;
		double gamma1 = playerState.EntFriction * settings.Frametime * settings.Maxspeed * accel;

		double lambdaVel = Length<Vector, 2>(playerState.Velocity);

		double cosTheta;

		if (gamma1 <= 2 * L)
		{
			cosTheta = ((targetVel * targetVel - lambdaVel * lambdaVel) / gamma1 - gamma1) / (2 * lambdaVel);
			return std::acos(cosTheta);
		}
		else
		{
			cosTheta = std::sqrt((targetVel * targetVel - L * L) / lambdaVel * lambdaVel);
			return std::acos(cosTheta);
		}
	}

	double CappedTheta(double wishspeed, const GameSettings& settings, const StrafeSettings& strafeSettings, const PlayerState& playerState)
	{
		double theta = MaxAccelTheta(wishspeed, settings, playerState);
		double speed = VectorFMESpeed(wishspeed, theta, playerState, settings);

		if (speed > strafeSettings.CappedSpeed)
			theta = TargetTheta(wishspeed, strafeSettings.CappedSpeed, settings, playerState);
		
		return theta;
	}

	double MaxAccelTheta(double wishspeed, const GameSettings& settings, const PlayerState& playerState)
	{
		double accel = Accel(wishspeed, playerState, settings);
		double accelspeed = AccelSpeed(accel, wishspeed, playerState, settings);
		if (accelspeed <= 0.0)
			return M_PI;

		if (IsZero<Vector, 2>(playerState.Velocity))
			return 0.0;

		double wishspeed_capped = WishspeedCap(wishspeed, playerState, settings);
		double tmp = wishspeed_capped - accelspeed;
		if (tmp <= 0.0)
			return M_PI / 2;

		double speed = Length<Vector, 2>(playerState.Velocity);
		if (tmp < speed)
			return std::acos(tmp / speed);

		return 0.0;
	}

	double MaxAngleTheta(double wishspeed, bool& safeguard_yaw, const GameSettings& settings, const PlayerState& playerState)
	{
		double speed = Length<Vector, 2>(playerState.Velocity);
		double accel = Accel(wishspeed, playerState, settings);
		double accelspeed = AccelSpeed(accel, wishspeed, playerState, settings);

		if (accelspeed <= 0.0)
		{
			double wishspeed_capped = (playerState.posType == PositionType::GROUND) ? wishspeed : settings.WishspeedCap;
			accelspeed *= -1;
			if (accelspeed >= speed)
			{
				if (wishspeed_capped >= speed)
					return 0.0;
				else
				{
					safeguard_yaw = true;
					return std::acos(wishspeed_capped
						/ speed); // The actual angle needs to be _less_ than this.
				}
			}
			else
			{
				if (wishspeed_capped >= speed)
					return std::acos(accelspeed / speed);
				else
				{
					safeguard_yaw = (wishspeed_capped <= accelspeed);
					return std::acos(
						std::min(accelspeed, wishspeed_capped)
						/ speed); // The actual angle needs to be _less_ than this if wishspeed_capped <= accelspeed.
				}
			}
		}
		else
		{
			if (accelspeed >= speed)
				return M_PI;
			else
				return std::acos(-1 * accelspeed / speed);
		}
	}

	double ButtonsPhi(Button button)
	{
		switch (button)
		{
		case Button::FORWARD:
			return 0;
		case Button::FORWARD_LEFT:
			return M_PI / 4;
		case Button::LEFT:
			return M_PI / 2;
		case Button::BACK_LEFT:
			return 3 * M_PI / 4;
		case Button::BACK:
			return -M_PI;
		case Button::BACK_RIGHT:
			return -3 * M_PI / 4;
		case Button::RIGHT:
			return -M_PI / 2;
		case Button::FORWARD_RIGHT:
			return -M_PI / 4;
		default:
			return 0;
		}
	}

	Button GetBestButtons(double theta, bool right)
	{
		if (theta < M_PI / 8)
			return Button::FORWARD;
		else if (theta < 3 * M_PI / 8)
			return right ? Button::FORWARD_RIGHT : Button::FORWARD_LEFT;
		else if (theta < 5 * M_PI / 8)
			return right ? Button::RIGHT : Button::LEFT;
		else if (theta < 7 * M_PI / 8)
			return right ? Button::BACK_RIGHT : Button::BACK_LEFT;
		else
			return Button::BACK;
	}

	void SideStrafeGeneral(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings,
		Button& usedButton,
		double strafe_theta,
		bool right,
		double& view_theta)
	{
		if (strafeSettings.UseGivenButtons)
		{
			if (!OnGround(playerState))
			{
				if (right)
					usedButton = strafeSettings.Buttons.AirRight;
				else
					usedButton = strafeSettings.Buttons.AirLeft;
			}
			else
			{
				if (right)
					usedButton = strafeSettings.Buttons.GroundRight;
				else
					usedButton = strafeSettings.Buttons.GroundLeft;
			}
		}
		else
		{
			usedButton = GetBestButtons(strafe_theta, right);
		}
		double phi = ButtonsPhi(usedButton);
		strafe_theta = right ? -strafe_theta : strafe_theta;

		double vel_theta = VelTheta(playerState, input);
		view_theta = NormalizeRad(vel_theta - phi + strafe_theta);
	}

	void MapSpeeds(ProcessedFrame& out, const GameSettings& settings)
	{
		if (out.Forward)
		{
			out.ForwardSpeed += settings.Maxspeed;
		}
		if (out.Back)
		{
			out.ForwardSpeed -= settings.Maxspeed;
		}
		if (out.Right)
		{
			out.SideSpeed += settings.Maxspeed;
		}
		if (out.Left)
		{
			out.SideSpeed -= settings.Maxspeed;
		}
	}

	void StrafeJump(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings)
	{
		// Jumpbug check
		if (playerState.CanJumpbug && input.Jumpbug)
		{
			frame.Jump = true;
			frame.ForceUnduck = true;
		}
		else if (playerState.posType == PositionType::GROUND && input.AutoJump)
		{
			frame.Jump = true;
		}
		else
		{
			frame.Jump = false;
		}

		if (!frame.Jump)
		{
			return;
		}

		frame.Processed = true;

		if (input.JumpType == 2)
		{
			// OE bhop
			frame.Yaw = NormalizeDeg(input.StrafeYaw);
			frame.Forward = true;
			MapSpeeds(frame, settings);
		}
		else if (input.JumpType == 1)
		{
			float speed = Length<Vector, 2>(playerState.Velocity);

			if (strafeSettings.UseAFH)
			{ // AFH
				frame.Yaw = input.StrafeYaw;
				frame.ForwardSpeed = -strafeSettings.AFHLength;
			}
			else
			{
				// ABH
				frame.Yaw = NormalizeDeg(input.StrafeYaw + 180);
			}
		}
		else if (input.JumpType == 3)
		{
			// Glitchless bhop
			frame.Yaw = NormalizeRad(Atan2(playerState.Velocity[1], playerState.Velocity[0])) * M_RAD2DEG;
			frame.Forward = true;
			MapSpeeds(frame, settings);
		}
		else
		{
			// Invalid jump type set
			frame.Processed = false;
		}
	}

	void StrafeVectorial(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings)
	{
		StrafeJump(frame, input, settings, playerState, strafeSettings);

		if (frame.Processed)
		{
			return;
		}

		ProcessedFrame dummy;
		Strafe(dummy, input, settings, playerState, strafeSettings);

		// If forward is pressed, strafing should occur
		if (dummy.Forward)
		{
			// Set move speeds to match the current yaw to produce the acceleration in direction thetaDeg
			double diff = (frame.Yaw - dummy.Yaw) * M_DEG2RAD;
			frame.ForwardSpeed = static_cast<float>(std::cos(diff) * settings.Maxspeed);
			frame.SideSpeed = static_cast<float>(std::sin(diff) * settings.Maxspeed);
			frame.Processed = true;
		}
	}

	void Strafe(ProcessedFrame& frame, const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings)
	{
		StrafeJump(frame, input, settings, playerState, strafeSettings);

		if (frame.Processed)
		{
			return;
		}

		double wishspeed = settings.Maxspeed;
		if (playerState.ReduceWishspeed)
			wishspeed *= 0.33333333f;

		Button usedButton = Button::FORWARD;
		bool strafed;
		strafed = true;

		double theta;
		bool safeguard_yaw = false;

		if (input.strafeType == StrafeType::MAXACCEL)
			theta = MaxAccelTheta(wishspeed, settings, playerState);
		else if (input.strafeType == StrafeType::MAXANGLE)
			theta = MaxAngleTheta(wishspeed, safeguard_yaw, settings, playerState);
		else if (input.strafeType == StrafeType::CAPPED)
			theta = CappedTheta(wishspeed, settings, strafeSettings, playerState);
		else if (input.strafeType == StrafeType::DIRECTION)
			theta = 0;

		// TODO: Add theta processing

		if (strafed)
		{
			frame.Forward = (usedButton == Button::FORWARD || usedButton == Button::FORWARD_LEFT
				|| usedButton == Button::FORWARD_RIGHT);
			frame.Back = (usedButton == Button::BACK || usedButton == Button::BACK_LEFT
				|| usedButton == Button::BACK_RIGHT);
			frame.Right = (usedButton == Button::RIGHT || usedButton == Button::FORWARD_RIGHT
				|| usedButton == Button::BACK_RIGHT);
			frame.Left = (usedButton == Button::LEFT || usedButton == Button::FORWARD_LEFT
				|| usedButton == Button::BACK_LEFT);
			frame.Processed = true;
			MapSpeeds(frame, settings);
		}
	}

	ProcessedFrame Strafe(const MovementInput& input, const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings)
	{
		ProcessedFrame frame;

		return frame;
	}

	void Friction(PlayerData& player, bool onground, const MovementVars& vars)
	{
		if (!onground)
			return;

		// Doing all this in floats, mismatch is too real otherwise.
		auto speed = Length<Vector, 2>(player.Velocity);
		if (speed < 0.1)
			return;

		auto friction = float{ vars.Friction * vars.EntFriction };
		auto control = (speed < vars.Stopspeed) ? vars.Stopspeed : speed;
		auto drop = control * friction * vars.Frametime;
		auto newspeed = std::max(speed - drop, 0.0);
		VecScale<Vector, 2>(player.Velocity, (newspeed / speed), player.Velocity);
	}

	bool LgagstJump(const GameSettings& settings, const PlayerState& playerState, const StrafeSettings& strafeSettings)
	{
		double vel = Length<Vector, 2>(playerState.Velocity);
		if (OnGround(playerState) && vel <= strafeSettings.LgagstMaxSpeed && vel >= strafeSettings.LgagstMinSpeed)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}