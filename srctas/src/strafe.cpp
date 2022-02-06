#include "strafe.hpp"
#include "strafe_utils.hpp"
#include <algorithm>
#include <cstddef>

namespace srctas
{
	void VectorFME(PlayerData& player,
		const MovementVars& vars,
		PositionType postype,
		double wishspeed,
		const Vector& a)
	{
		bool onground = (postype == PositionType::GROUND);
		double wishspeed_capped = onground ? wishspeed : 30; // TODO: Fix for portal
		double tmp = wishspeed_capped - DotProduct<Vector, 2>(player.Velocity, a);
		if (tmp <= 0.0)
			return;

		double accel = onground ? vars.Accelerate : vars.Airaccelerate;
		double accelspeed = accel * wishspeed * vars.EntFriction * vars.Frametime;
		if (accelspeed <= tmp)
			tmp = accelspeed;

		player.Velocity[0] += static_cast<float>(a[0] * tmp);
		player.Velocity[1] += static_cast<float>(a[1] * tmp);
	}

	void CheckVelocity(PlayerData& player, const MovementVars& vars)
	{
		for (std::size_t i = 0; i < 3; ++i)
		{
			if (player.Velocity[i] > vars.Maxvelocity)
				player.Velocity[i] = vars.Maxvelocity;
			if (player.Velocity[i] < -vars.Maxvelocity)
				player.Velocity[i] = -vars.Maxvelocity;
		}
	}

	double TargetTheta(const PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		double target)
	{
		double accel = onground ? vars.Accelerate : vars.Airaccelerate;
		double L = vars.WishspeedCap;
		double gamma1 = vars.EntFriction * vars.Frametime * vars.Maxspeed * accel;

		PlayerData copy = player;
		double lambdaVel = Length<Vector, 2>(copy.Velocity);

		double cosTheta;

		if (gamma1 <= 2 * L)
		{
			cosTheta = ((target * target - lambdaVel * lambdaVel) / gamma1 - gamma1) / (2 * lambdaVel);
			return std::acos(cosTheta);
		}
		else
		{
			cosTheta = std::sqrt((target * target - L * L) / lambdaVel * lambdaVel);
			return std::acos(cosTheta);
		}
	}

	double MaxAccelWithCapIntoYawTheta(const PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		double vel_yaw,
		double yaw)
	{
		if (!IsZero<Vector, 2>(player.Velocity))
			vel_yaw = Atan2(player.Velocity.y, player.Velocity.x);

		double theta = MaxAccelTheta(player, vars, onground, wishspeed);

		Vector avec(std::cos(theta), std::sin(theta), 0);
		PlayerData vel;
		vel.Velocity.x = Length<Vector, 2>(player.Velocity);
		VectorFME(vel, vars, onground, wishspeed, avec);

		if (Length<Vector, 2>(vel.Velocity) > tas_strafe_capped_limit.GetFloat())
			theta = TargetTheta(player, vars, onground, wishspeed, tas_strafe_capped_limit.GetFloat());

		return std::copysign(theta, NormalizeRad(yaw - vel_yaw));
	}

	double MaxAccelTheta(const PlayerData& player, const MovementVars& vars, bool onground, double wishspeed)
	{
		double accel = onground ? vars.Accelerate : vars.Airaccelerate;
		double accelspeed = accel * wishspeed * vars.EntFriction * vars.Frametime;
		if (accelspeed <= 0.0)
			return M_PI;

		if (IsZero<Vector, 2>(player.Velocity))
			return 0.0;

		double wishspeed_capped = onground ? wishspeed : vars.WishspeedCap;
		double tmp = wishspeed_capped - accelspeed;
		if (tmp <= 0.0)
			return M_PI / 2;

		double speed = Length<Vector, 2>(player.Velocity);
		if (tmp < speed)
			return std::acos(tmp / speed);

		return 0.0;
	}

	double MaxAccelIntoYawTheta(const PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		double vel_yaw,
		double yaw)
	{
		if (!IsZero<Vector, 2>(player.Velocity))
			vel_yaw = Atan2(player.Velocity.y, player.Velocity.x);

		double theta = MaxAccelTheta(player, vars, onground, wishspeed);
		if (theta == 0.0 || theta == M_PI)
			return NormalizeRad(yaw - vel_yaw + theta);
		return std::copysign(theta, NormalizeRad(yaw - vel_yaw));
	}

	double MaxAngleTheta(const PlayerData& player,
		const MovementVars& vars,
		bool onground,
		double wishspeed,
		bool& safeguard_yaw)
	{
		safeguard_yaw = false;
		double speed = Length<Vector, 2>(player.Velocity);
		double accel = onground ? vars.Accelerate : vars.Airaccelerate;
		double accelspeed = accel * wishspeed * vars.EntFriction * vars.Frametime;

		if (accelspeed <= 0.0)
		{
			double wishspeed_capped = onground ? wishspeed : vars.WishspeedCap;
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

	void SideStrafeGeneral(const PlayerData& player,
		const MovementVars& vars,
		bool onground,
		const StrafeButtons& strafeButtons,
		bool useGivenButtons,
		Button& usedButton,
		double vel_yaw,
		double theta,
		bool right,
		double& yaw)
	{
		if (useGivenButtons)
		{
			if (!onground)
			{
				if (right)
					usedButton = strafeButtons.AirRight;
				else
					usedButton = strafeButtons.AirLeft;
			}
			else
			{
				if (right)
					usedButton = strafeButtons.GroundRight;
				else
					usedButton = strafeButtons.GroundLeft;
			}
		}
		else
		{
			usedButton = GetBestButtons(theta, right);
		}
		double phi = ButtonsPhi(usedButton);
		theta = right ? -theta : theta;

		if (!IsZero<Vector, 2>(player.Velocity))
			vel_yaw = Atan2(player.Velocity.y, player.Velocity.x);

		yaw = NormalizeRad(vel_yaw - phi + theta);
	}

	void MapSpeeds(ProcessedFrame& out, const MovementVars& vars)
	{
		if (out.Forward)
		{
			out.ForwardSpeed += vars.Maxspeed;
		}
		if (out.Back)
		{
			out.ForwardSpeed -= vars.Maxspeed;
		}
		if (out.Right)
		{
			out.SideSpeed += vars.Maxspeed;
		}
		if (out.Left)
		{
			out.SideSpeed -= vars.Maxspeed;
		}
	}

	bool StrafeJump(bool jumped, PlayerData& player, const MovementVars& vars, ProcessedFrame& out, bool yawChanged)
	{
		bool rval = false;
		if (!jumped)
		{
			rval = false;
		}
		else
		{
			out.Jump = true;
			out.Processed = true;

			if (yawChanged && !tas_strafe_allow_jump_override.GetBool())
			{
				rval = true;
			}
			else if (tas_strafe_jumptype.GetInt() == 2)
			{
				// OE bhop
				out.Yaw = NormalizeDeg(tas_strafe_yaw.GetFloat());
				out.Forward = true;
				MapSpeeds(out, vars);

				rval = true;
			}
			else if (tas_strafe_jumptype.GetInt() == 1)
			{
				float cap = vars.Maxspeed * ((player.Ducking || (vars.Maxspeed == 320)) ? 0.1 : 0.5);
				float speed = Length<Vector, 2>(player.Velocity);

				if (speed >= cap)
				{
					// Above ABH speed
					if (tas_strafe_afh.GetBool())
					{ // AFH
						out.Yaw = tas_strafe_yaw.GetFloat();
						out.ForwardSpeed = -tas_strafe_afh_length.GetFloat();
						rval = true;
					}
					else
					{
						// ABH
						out.Yaw = NormalizeDeg(tas_strafe_yaw.GetFloat() + 180);
						rval = true;
					}
				}
				else
				{
					// Below ABH speed, dont do anything
					rval = false;
				}
			}
			else if (tas_strafe_jumptype.GetInt() == 3)
			{
				// Glitchless bhop
				const Vector vel = player.Velocity;
				out.Yaw = NormalizeRad(Atan2(player.Velocity[1], player.Velocity[0])) * M_RAD2DEG;
				out.Forward = true;
				MapSpeeds(out, vars);

				rval = true;
			}
			else
			{
				// Invalid jump type set
				out.Processed = false;
				rval = false;
			}
		}

		// Jumpbug check
		if (out.Jump && !player.Ducking && player.DuckPressed && tas_strafe_autojb.GetBool())
			out.ForceUnduck = true;

		return rval;
	}

	void StrafeVectorial(PlayerData& player,
		const MovementVars& vars,
		bool jumped,
		StrafeType type,
		StrafeDir dir,
		double target_yaw,
		double vel_yaw,
		ProcessedFrame& out,
		bool yawChanged)
	{
		if (StrafeJump(jumped, player, vars, out, yawChanged))
		{
			return;
		}

		ProcessedFrame dummy;
		Strafe(
			player,
			vars,
			jumped,
			type,
			dir,
			target_yaw,
			vel_yaw,
			dummy,
			StrafeButtons(),
			true); // Get the desired strafe direction by calling the Strafe function while using forward strafe buttons

		// If forward is pressed, strafing should occur
		if (dummy.Forward)
		{
			out.Yaw = vel_yaw;

			// Set move speeds to match the current yaw to produce the acceleration in direction thetaDeg
			double thetaDeg = dummy.Yaw;
			double diff = (out.Yaw - thetaDeg) * M_DEG2RAD;
			out.ForwardSpeed = static_cast<float>(std::cos(diff) * vars.Maxspeed);
			out.SideSpeed = static_cast<float>(std::sin(diff) * vars.Maxspeed);
			out.Processed = true;
		}
	}

	bool Strafe(PlayerData& player,
		const MovementVars& vars,
		bool jumped,
		StrafeType type,
		StrafeDir dir,
		double target_yaw,
		double vel_yaw,
		ProcessedFrame& out,
		const StrafeButtons& strafeButtons,
		bool useGivenButtons)
	{
		//DevMsg("[Strafing] ducking = %d\n", (int)ducking);
		if (StrafeJump(jumped, player, vars, out,
			false)) // yawChanged == false when calling this function
		{
			return vars.OnGround;
		}

		double wishspeed = vars.Maxspeed;
		if (vars.ReduceWishspeed)
			wishspeed *= 0.33333333f;

		Button usedButton = Button::FORWARD;
		bool strafed;
		strafed = true;

		switch (dir)
		{
		case StrafeDir::YAW:
			if (type == StrafeType::MAXACCEL)
				out.Yaw = YawStrafeMaxAccel(player,
					vars,
					vars.OnGround,
					wishspeed,
					strafeButtons,
					useGivenButtons,
					usedButton,
					vel_yaw * M_DEG2RAD,
					target_yaw * M_DEG2RAD)
				* M_RAD2DEG;
			else if (type == StrafeType::MAXANGLE)
				out.Yaw = YawStrafeMaxAngle(player,
					vars,
					vars.OnGround,
					wishspeed,
					strafeButtons,
					useGivenButtons,
					usedButton,
					vel_yaw * M_DEG2RAD,
					target_yaw * M_DEG2RAD)
				* M_RAD2DEG;
			else if (type == StrafeType::CAPPED)
				out.Yaw = YawStrafeCapped(player,
					vars,
					vars.OnGround,
					wishspeed,
					strafeButtons,
					useGivenButtons,
					usedButton,
					vel_yaw * M_DEG2RAD,
					target_yaw * M_DEG2RAD)
				* M_RAD2DEG;
			else if (type == StrafeType::DIRECTION)
				out.Yaw = target_yaw;
			break;
		default:
			strafed = false;
			break;
		}

		if (strafed)
		{
			out.Forward = (usedButton == Button::FORWARD || usedButton == Button::FORWARD_LEFT
				|| usedButton == Button::FORWARD_RIGHT);
			out.Back = (usedButton == Button::BACK || usedButton == Button::BACK_LEFT
				|| usedButton == Button::BACK_RIGHT);
			out.Right = (usedButton == Button::RIGHT || usedButton == Button::FORWARD_RIGHT
				|| usedButton == Button::BACK_RIGHT);
			out.Left = (usedButton == Button::LEFT || usedButton == Button::FORWARD_LEFT
				|| usedButton == Button::BACK_LEFT);
			out.Processed = true;
			MapSpeeds(out, vars);
		}

		return vars.OnGround;
	}

	void Friction(PlayerData& player, bool onground, const MovementVars& vars)
	{
		if (!onground)
			return;

		// Doing all this in floats, mismatch is too real otherwise.
		auto speed = Length<Vector, 2>(player.Velocity);
		if (speed < 0.1)
			return;

		auto friction = double{ vars.Friction * vars.EntFriction };
		auto control = (speed < vars.Stopspeed) ? vars.Stopspeed : speed;
		auto drop = control * friction * vars.Frametime;
		auto newspeed = std::max(speed - drop, 0.0);
		VecScale<Vector, 2>(player.Velocity, (newspeed / speed), player.Velocity);
	}

	bool LgagstJump(PlayerData& player, const MovementVars& vars)
	{
		double vel = Length<Vector, 2>(player.Velocity);
		if (vars.OnGround && vel <= tas_strafe_lgagst_max.GetFloat() && vel >= tas_strafe_lgagst_min.GetFloat())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}