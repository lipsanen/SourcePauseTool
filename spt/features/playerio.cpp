#include "stdafx.h"
#include "..\OrangeBox\spt-serverplugin.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\aim.hpp"
#include "..\strafestuff.hpp"
#include "..\OrangeBox\cvars.hpp"
#include "..\utils\game_detection.hpp"
#include "..\utils\math.hpp"
#include "playerio.hpp"

#include "mathlib\vmatrix.h"

#undef max
#undef min

PlayerIOFeature _playerio;
static void* cinput_thisptr = nullptr;

bool PlayerIOFeature::ShouldLoadFeature()
{
	return GetEngine() != nullptr;
}

void PlayerIOFeature::InitHooks()
{
	HOOK_FUNCTION_WITH_CALLBACK(client, CreateMove, CreateMove_callback);
	HOOK_FUNCTION(client, GetButtonBits);
	FIND_PATTERN(client, CalcAbsoluteVelocity);
	FIND_PATTERN_WITH_CALLBACK(client, GetGroundEntity, GetGroundEntity_callback);
	FIND_PATTERN_WITH_CALLBACK(client, MiddleOfCAM_Think, MiddleOfCAM_Think_callback);
	FIND_PATTERN(server, PlayerRunCommand);
}

void PlayerIOFeature::LoadFeature()
{
	playerioAddressesWereFound = PlayerIOAddressesFound();
	if (utils::DoesGameLookLikeHLS())
	{
		sizeofCUserCmd = 64; // Is missing a CUtlVector
	}
	else
	{
		sizeofCUserCmd = 84;
	}

	offM_vecAbsVelocity = 0;
	offM_afPhysicsFlags = 0;
	offM_moveCollide = 0;
	offM_moveType = 0;
	offM_collisionGroup = 0;
	offM_vecPunchAngle = 0;
	offM_vecPunchAngleVel = 0;

	if(ORIG_PlayerRunCommand)
	{
		int ptnNumber = GetPatternIndex((PVOID*)&ORIG_PlayerRunCommand);
		switch (ptnNumber)
		{
		case 0:
			offM_vecAbsVelocity = 476;
			offM_afPhysicsFlags = 2724;
			offM_moveCollide = 307;
			offM_moveType = 306;
			offM_collisionGroup = 420;
			offM_vecPunchAngle = 2192;
			offM_vecPunchAngleVel = 2200;
			break;
		case 1:
			offM_vecAbsVelocity = 476;
			break;

		case 2:
			offM_vecAbsVelocity = 532;
			break;

		case 3:
			offM_vecAbsVelocity = 532;
			break;

		case 4:
			offM_vecAbsVelocity = 532;
			break;

		case 5:
			offM_vecAbsVelocity = 476;
			break;

		case 6:
			offM_vecAbsVelocity = 532;
			break;

		case 7:
			offM_vecAbsVelocity = 532;
			break;

		case 8:
			offM_vecAbsVelocity = 592;
			break;

		case 9:
			offM_vecAbsVelocity = 556;
			break;

		case 10:
			offM_vecAbsVelocity = 364;
			break;

		case 12:
			offM_vecAbsVelocity = 476;
			break;
		}
	}
	else
	{
		DevWarning("[server dll] Could not find PlayerRunCommand!\n");
		Warning("_y_spt_getvel has no effect.\n");
	}
}

void PlayerIOFeature::UnloadFeature() {}

void PlayerIOFeature::HandleAiming(float* va, bool& yawChanged)
{
	// Use tas_aim stuff for tas_strafe_version >= 4
	if (tas_strafe_version.GetInt() >= 4)
	{
		aim::UpdateView(va[PITCH], va[YAW]);
	}

	double pitchSpeed = atof(_y_spt_pitchspeed.GetString()), yawSpeed = atof(_y_spt_yawspeed.GetString());

	if (pitchSpeed != 0.0f)
		va[PITCH] += pitchSpeed;
	if (setPitch.set)
	{
		setPitch.set = DoAngleChange(va[PITCH], setPitch.angle);
	}

	if (yawSpeed != 0.0f)
	{
		va[YAW] += yawSpeed;
	}
	if (setYaw.set)
	{
		yawChanged = true;
		setYaw.set = DoAngleChange(va[YAW], setYaw.angle);
	}
}

bool PlayerIOFeature::DoAngleChange(float& angle, float target)
{
	float normalizedDiff = utils::NormalizeDeg(target - angle);
	if (std::abs(normalizedDiff) > _y_spt_anglesetspeed.GetFloat())
	{
		angle += std::copysign(_y_spt_anglesetspeed.GetFloat(), normalizedDiff);
		return true;
	}
	else
	{
		angle = target;
		return false;
	}
}

Strafe::MovementVars PlayerIOFeature::GetMovementVars()
{
	auto vars = Strafe::MovementVars();

	if (!playerioAddressesWereFound || cinput_thisptr == nullptr)
	{
		return vars;
	}

	auto player = ORIG_GetLocalPlayer();
	auto maxspeed = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offMaxspeed);

	auto pl = GetPlayerData();
	vars.OnGround = Strafe::GetPositionType(pl, pl.Ducking ? Strafe::HullType::DUCKED : Strafe::HullType::NORMAL)
	                == Strafe::PositionType::GROUND;
	bool ground; // for backwards compatibility with old bugs

	if (tas_strafe_version.GetInt() <= 1)
	{
		ground = IsGroundEntitySet();
	}
	else
	{
		ground = vars.OnGround;
	}

	if (tas_force_onground.GetBool())
	{
		ground = true;
		vars.OnGround = true;
	}

	vars.ReduceWishspeed = ground && GetFlagsDucking();
	vars.Accelerate = _sv_accelerate->GetFloat();

	if (tas_force_airaccelerate.GetString()[0] != '\0')
		vars.Airaccelerate = tas_force_airaccelerate.GetFloat();
	else
		vars.Airaccelerate = _sv_airaccelerate->GetFloat();

	vars.EntFriction = 1;
	vars.Frametime = 0.015f;
	vars.Friction = _sv_friction->GetFloat();
	vars.Maxspeed = (maxspeed > 0) ? std::min(maxspeed, _sv_maxspeed->GetFloat()) : _sv_maxspeed->GetFloat();
	vars.Stopspeed = _sv_stopspeed->GetFloat();

	if (tas_force_wishspeed_cap.GetString()[0] != '\0')
		vars.WishspeedCap = tas_force_wishspeed_cap.GetFloat();
	else
		vars.WishspeedCap = 30;

	extern IServerUnknown* GetServerPlayer();
	auto server_player = GetServerPlayer();

	auto previouslyPredictedOrigin =
	    reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(server_player) + offServerPreviouslyPredictedOrigin);
	auto absOrigin = reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(server_player) + offServerAbsOrigin);
	bool gameCodeMovedPlayer = (*previouslyPredictedOrigin != *absOrigin);

	vars.EntFriction =
	    *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(server_player) + offServerSurfaceFriction);

	if (gameCodeMovedPlayer)
	{
		if (tas_reset_surface_friction.GetBool())
		{
			vars.EntFriction = 1.0f;
		}

		if (pl.Velocity.z <= 140.f)
		{
			if (ground)
			{
				// TODO: This should check the real surface friction.
				vars.EntFriction = 1.0f;
			}
			else if (pl.Velocity.z > 0.f)
			{
				vars.EntFriction = 0.25f;
			}
		}
	}

	vars.EntGravity = 1.0f;
	vars.Maxvelocity = _sv_maxvelocity->GetFloat();
	vars.Gravity = _sv_gravity->GetFloat();
	vars.Stepsize = _sv_stepsize->GetFloat();
	vars.Bounce = _sv_bounce->GetFloat();

	vars.CantJump = false;
	// This will report air on the first frame of unducking and report ground on the last one.
	if ((*reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(player) + offDucking)) && GetFlagsDucking())
	{
		vars.CantJump = true;
	}

	auto djt = (*reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offDuckJumpTime));
	djt -= vars.Frametime * 1000;
	djt = std::max(0.f, djt);
	float flDuckMilliseconds = std::max(0.0f, 1000.f - djt);
	float flDuckSeconds = flDuckMilliseconds * 0.001f;
	if (flDuckSeconds > 0.2)
	{
		djt = 0;
	}
	if (djt > 0)
	{
		vars.CantJump = true;
	}

	return vars;
}

void __fastcall PlayerIOFeature::HOOKED_CreateMove_Func(void* thisptr,
                                                        int edx,
                                                        int sequence_number,
                                                        float input_sample_frametime,
                                                        bool active)
{
	auto m_pCommands = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(thisptr) + offM_pCommands);
	pCmd = m_pCommands + sizeofCUserCmd * (sequence_number % 90);

	ORIG_CreateMove(thisptr, edx, sequence_number, input_sample_frametime, active);

	pCmd = 0;
}

void __fastcall PlayerIOFeature::HOOKED_CreateMove(void* thisptr,
                                                   int edx,
                                                   int sequence_number,
                                                   float input_sample_frametime,
                                                   bool active)
{
	_playerio.HOOKED_CreateMove_Func(thisptr, edx, sequence_number, input_sample_frametime, active);
}

int __fastcall PlayerIOFeature::HOOKED_GetButtonBits_Func(void* thisptr, int edx, int bResetState)
{
	int rv = ORIG_GetButtonBits(thisptr, edx, bResetState);

	if (bResetState == 1)
	{
		static bool duckPressed = false;

		if (duckspam)
		{
			if (duckPressed)
				duckPressed = false;
			else
			{
				duckPressed = true;
				rv |= (1 << 2); // IN_DUCK
			}
		}
		else
			duckPressed = false;

		if (forceJump)
		{
			forceJump = false;
			rv |= (1 << 1); // IN_JUMP
		}

		if (forceUnduck)
		{
			rv ^= (1 << 2); // IN_DUCK
			forceUnduck = false;
		}
	}

	return rv;
}

int __fastcall PlayerIOFeature::HOOKED_GetButtonBits(void* thisptr, int edx, int bResetState)
{
	return _playerio.HOOKED_GetButtonBits_Func(thisptr, edx, bResetState);
}

void PlayerIOFeature::GetGroundEntity_callback(patterns::PatternWrapper* found, int index)
{
	if (!_playerio.ORIG_GetGroundEntity)
		return;

	switch (index)
	{
	case 0:
		_playerio.offMaxspeed = 4136;
		_playerio.offFlags = 736;
		_playerio.offAbsVelocity = 248;
		_playerio.offDucking = 3545;
		_playerio.offDuckJumpTime = 3552;
		_playerio.offServerSurfaceFriction = 3812;
		_playerio.offServerPreviouslyPredictedOrigin = 3692;
		_playerio.offServerAbsOrigin = 580;
		break;

	case 1:
		_playerio.offMaxspeed = 4076;
		_playerio.offFlags = 732;
		_playerio.offAbsVelocity = 244;
		_playerio.offDucking = 3489;
		_playerio.offDuckJumpTime = 3496;
		_playerio.offServerSurfaceFriction = 3752;
		_playerio.offServerPreviouslyPredictedOrigin = 3628;
		_playerio.offServerAbsOrigin = 580;
		break;

	case 2:
		_playerio.offMaxspeed = 4312;
		_playerio.offFlags = 844;
		_playerio.offAbsVelocity = 300;
		_playerio.offDucking = 3713;
		_playerio.offDuckJumpTime = 3720;
		_playerio.offServerSurfaceFriction = 3872;
		_playerio.offServerPreviouslyPredictedOrigin = 3752;
		_playerio.offServerAbsOrigin = 636;
		break;

	case 3:
		_playerio.offMaxspeed = 4320;
		_playerio.offFlags = 844;
		_playerio.offAbsVelocity = 300;
		_playerio.offDucking = 3721;
		_playerio.offDuckJumpTime = 3728;
		_playerio.offServerSurfaceFriction = 3872;
		_playerio.offServerPreviouslyPredictedOrigin = 3752;
		_playerio.offServerAbsOrigin = 636;
		break;
	default:
		_playerio.offMaxspeed = 0;
		_playerio.offFlags = 0;
		_playerio.offAbsVelocity = 0;
		_playerio.offDucking = 0;
		_playerio.offDuckJumpTime = 0;
		_playerio.offServerSurfaceFriction = 0;
		_playerio.offServerPreviouslyPredictedOrigin = 0;
		_playerio.offServerAbsOrigin = 0;
		Warning("GetGroundEntity did not contain matching if statement for pattern!\n");
		break;
	}
}

void PlayerIOFeature::MiddleOfCAM_Think_callback(patterns::PatternWrapper* found, int index)
{
	if (!_playerio.ORIG_MiddleOfCAM_Think)
		return;

	switch (index)
	{
	case 0:
		_playerio.ORIG_GetLocalPlayer =
		    (_GetLocalPlayer)(*reinterpret_cast<uintptr_t*>(_playerio.ORIG_MiddleOfCAM_Think + 29)
		                      + _playerio.ORIG_MiddleOfCAM_Think + 33);
		break;

	case 1:
		_playerio.ORIG_GetLocalPlayer =
		    (_GetLocalPlayer)(*reinterpret_cast<uintptr_t*>(_playerio.ORIG_MiddleOfCAM_Think + 30)
		                      + _playerio.ORIG_MiddleOfCAM_Think + 34);
		break;

	case 2:
		_playerio.ORIG_GetLocalPlayer =
		    (_GetLocalPlayer)(*reinterpret_cast<uintptr_t*>(_playerio.ORIG_MiddleOfCAM_Think + 21)
		                      + _playerio.ORIG_MiddleOfCAM_Think + 25);
		break;

	case 3:
		_playerio.ORIG_GetLocalPlayer =
		    (_GetLocalPlayer)(*reinterpret_cast<uintptr_t*>(_playerio.ORIG_MiddleOfCAM_Think + 23)
		                      + _playerio.ORIG_MiddleOfCAM_Think + 27);
		break;
	default:
		_playerio.ORIG_GetLocalPlayer = nullptr;
		break;
	}

	if (_playerio.ORIG_GetLocalPlayer)
		DevMsg("[client.dll] Found GetLocalPlayer at %p.\n", _playerio.ORIG_GetLocalPlayer);
}

void PlayerIOFeature::CreateMove_callback(patterns::PatternWrapper* found, int index)
{
	if (!_playerio.ORIG_CreateMove)
		return;

	switch (index)
	{
	case 0:
		_playerio.offM_pCommands = 180;
		_playerio.offForwardmove = 24;
		_playerio.offSidemove = 28;
		break;

	case 1:
		_playerio.offM_pCommands = 196;
		_playerio.offForwardmove = 24;
		_playerio.offSidemove = 28;
		break;

	case 2:
		_playerio.offM_pCommands = 196;
		_playerio.offForwardmove = 24;
		_playerio.offSidemove = 28;
		break;

	case 3:
		_playerio.offM_pCommands = 196;
		_playerio.offForwardmove = 24;
		_playerio.offSidemove = 28;
		break;

	case 4:
		_playerio.offM_pCommands = 196;
		_playerio.offForwardmove = 24;
		_playerio.offSidemove = 28;
		break;

	case 5:
		_playerio.offM_pCommands = 196;
		_playerio.offForwardmove = 24;
		_playerio.offSidemove = 28;
		break;
	default:
		_playerio.offM_pCommands = 0;
		_playerio.offForwardmove = 0;
		_playerio.offSidemove = 0;
		break;
	}
}

bool PlayerIOFeature::GetFlagsDucking()
{
	if (!ORIG_GetLocalPlayer)
		return false;
	return (*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(ORIG_GetLocalPlayer()) + offFlags)) & FL_DUCKING;
}

int PlayerIOFeature::GetPlayerFlags()
{
	if (!ORIG_GetLocalPlayer)
		return 0;
	return (*reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(ORIG_GetLocalPlayer()) + offFlags));
}

Strafe::PlayerData PlayerIOFeature::GetPlayerData()
{
	if (!playerioAddressesWereFound)
		return Strafe::PlayerData();

	Strafe::PlayerData data;
	const int IN_DUCK = 1 << 2;

	data.Ducking = GetFlagsDucking();
	data.DuckPressed = (ORIG_GetButtonBits(cinput_thisptr, 0, 0) & IN_DUCK);
	data.UnduckedOrigin =
	    *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + offServerAbsOrigin);
	data.Velocity = GetPlayerVelocity();
	data.Basevelocity = Vector();

	if (data.Ducking)
	{
		data.UnduckedOrigin.z -= 36;

		if (tas_strafe_use_tracing.GetBool() && Strafe::CanUnduck(data))
			data.Ducking = false;
	}

	return data;
}

Vector PlayerIOFeature::GetPlayerVelocity()
{
	if (!ORIG_GetLocalPlayer)
		return Vector();
	auto player = ORIG_GetLocalPlayer();
	ORIG_CalcAbsoluteVelocity(player, 0);
	float* vel = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offAbsVelocity);

	return Vector(vel[0], vel[1], vel[2]);
}

Vector PlayerIOFeature::GetPlayerEyePos()
{
	Vector rval = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + offServerAbsOrigin);
	constexpr float duckOffset = 28;
	constexpr float standingOffset = 64;

	if (GetFlagsDucking())
	{
		rval.z += duckOffset;
	}
	else
	{
		rval.z += standingOffset;
	}

	return rval;
}

bool PlayerIOFeature::IsGroundEntitySet()
{
	if (ORIG_GetGroundEntity == nullptr || ORIG_GetLocalPlayer == nullptr)
		return false;

	auto player = ORIG_GetLocalPlayer();
	return (ORIG_GetGroundEntity(player, 0) != NULL); // TODO: This should really be a proper check.
}

bool PlayerIOFeature::TryJump()
{
	const int IN_JUMP = (1 << 1);
	return ORIG_GetButtonBits(cinput_thisptr, 0, 0) & IN_JUMP;
}

bool PlayerIOFeature::PlayerIOAddressesFound()
{
	return ORIG_GetGroundEntity && ORIG_CreateMove && ORIG_GetButtonBits && ORIG_GetLocalPlayer
	       && ORIG_CalcAbsoluteVelocity && ORIG_MiddleOfCAM_Think && _sv_airaccelerate && _sv_accelerate
	       && _sv_friction && _sv_maxspeed && _sv_stopspeed && FoundEngineServer();
}

void PlayerIOFeature::SetTASInput(float* va, const Strafe::ProcessedFrame& out)
{
	// This bool is set if strafing should occur
	if (out.Processed)
	{
		if (out.Jump && tas_strafe_jumptype.GetInt() > 0)
		{
			aim::SetJump();
		}

		// Apply jump and unduck regardless of whether we are strafing
		forceUnduck = out.ForceUnduck;
		forceJump = out.Jump;

		*reinterpret_cast<float*>(pCmd + offForwardmove) = out.ForwardSpeed;
		*reinterpret_cast<float*>(pCmd + offSidemove) = out.SideSpeed;
		va[YAW] = static_cast<float>(out.Yaw);
	}
}

double PlayerIOFeature::GetDuckJumpTime()
{
	if (!ORIG_GetLocalPlayer)
		return 0;

	auto player = ORIG_GetLocalPlayer();
	return *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + offDuckJumpTime);
}


int PlayerIOFeature::GetPlayerPhysicsFlags() const
{
	auto player = GetServerPlayer();
	if (!player || offM_afPhysicsFlags == 0)
		return -1;
	else
		return *reinterpret_cast<int*>(((int)GetServerPlayer() + offM_afPhysicsFlags));
}

int PlayerIOFeature::GetPlayerMoveType() const
{
	auto player = GetServerPlayer();
	if (!player || offM_moveType == 0)
		return -1;
	else
		return *reinterpret_cast<int*>(((int)GetServerPlayer() + offM_moveType)) & 0xF;
}

int PlayerIOFeature::GetPlayerMoveCollide() const
{
	auto player = GetServerPlayer();
	if (!player || offM_moveCollide == 0)
		return -1;
	else
		return *reinterpret_cast<int*>(((int)GetServerPlayer() + offM_moveCollide)) & 0x7;
}

int PlayerIOFeature::GetPlayerCollisionGroup() const
{
	auto player = GetServerPlayer();
	if (!player || offM_collisionGroup == 0)
		return -1;
	else
		return *reinterpret_cast<int*>(((int)GetServerPlayer() + offM_collisionGroup));
}


#if defined(OE)
static void DuckspamDown()
#else
static void DuckspamDown(const CCommand& args)
#endif
{
	_playerio.EnableDuckspam();
}
static ConCommand DuckspamDown_Command("+y_spt_duckspam", DuckspamDown, "Enables the duckspam.");

#if defined(OE)
static void DuckspamUp()
#else
static void DuckspamUp(const CCommand& args)
#endif
{
	_playerio.DisableDuckspam();
}
static ConCommand DuckspamUp_Command("-y_spt_duckspam", DuckspamUp, "Disables the duckspam.");

CON_COMMAND(_y_spt_setpitch, "Sets the pitch. Usage: _y_spt_setpitch <pitch>")
{
#if defined(OE)
	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() != 2)
	{
		Msg("Usage: _y_spt_setpitch <pitch>\n");
		return;
	}

	_playerio.SetPitch(atof(args.Arg(1)));
}

CON_COMMAND(_y_spt_setyaw, "Sets the yaw. Usage: _y_spt_setyaw <yaw>")
{
#if defined(OE)
	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() != 2)
	{
		Msg("Usage: _y_spt_setyaw <yaw>\n");
		return;
	}

	_playerio.SetYaw(atof(args.Arg(1)));
}

CON_COMMAND(_y_spt_resetpitchyaw, "Resets pitch/yaw commands.")
{
	_playerio.ResetPitchYawCommands();
}

CON_COMMAND(_y_spt_setangles, "Sets the angles. Usage: _y_spt_setangles <pitch> <yaw>")
{
#if defined(OE)
	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() != 3)
	{
		Msg("Usage: _y_spt_setangles <pitch> <yaw>\n");
		return;
	}

	_playerio.SetPitch(atof(args.Arg(1)));
	_playerio.SetYaw(atof(args.Arg(2)));
}

CON_COMMAND(_y_spt_getvel, "Gets the last velocity of the player.")
{
	const Vector vel = _playerio.GetPlayerVelocity();

	Warning("Velocity (x, y, z): %f %f %f\n", vel.x, vel.y, vel.z);
	Warning("Velocity (xy): %f\n", vel.Length2D());
}

CON_COMMAND(_y_spt_setangle,
            "Sets the yaw/pitch angle required to look at the given position from player's current position.")
{
#if defined(OE)
	if (!engine)
		return;

	ArgsWrapper args(engine.get());
#endif
	Vector target;

	if (args.ArgC() > 3)
	{
		target.x = atof(args.Arg(1));
		target.y = atof(args.Arg(2));
		target.z = atof(args.Arg(3));

		Vector player_origin = _playerio.GetPlayerEyePos();
		Vector diff = (target - player_origin);
		QAngle angles;
		VectorAngles(diff, angles);
		_playerio.SetPitch(angles[PITCH]);
		_playerio.SetYaw(angles[YAW]);
	}
}

#if SSDK2007
// TODO: remove fixed offsets.

CON_COMMAND(y_spt_find_portals, "Yes")
{
	if (_playerio.offServerAbsOrigin == 0)
		return;

	auto engine_server = GetEngine();

	for (int i = 0; i < MAX_EDICTS; ++i)
	{
		auto ent = engine_server->PEntityOfEntIndex(i);

		if (ent && !ent->IsFree() && !strcmp(ent->GetClassName(), "prop_portal"))
		{
			auto& origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(ent->GetUnknown())
			                                          + _playerio.offServerAbsOrigin);

			Msg("SPT: There's a portal with index %d at %.8f %.8f %.8f.\n",
			    i,
			    origin.x,
			    origin.y,
			    origin.z);
		}
	}
}

void calculate_offset_player_pos(edict_t* saveglitch_portal, Vector& new_player_origin, QAngle& new_player_angles)
{
	// Here we make sure that the eye position and the eye angles match up.
	const Vector view_offset(0, 0, 64);
	auto& player_origin =
	    *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + _playerio.offServerAbsOrigin);
	auto& player_angles = *reinterpret_cast<QAngle*>(reinterpret_cast<uintptr_t>(GetServerPlayer()) + 2568);

	auto& matrix = *reinterpret_cast<VMatrix*>(reinterpret_cast<uintptr_t>(saveglitch_portal->GetUnknown()) + 1072);

	auto eye_origin = player_origin + view_offset;
	auto new_eye_origin = matrix * eye_origin;
	new_player_origin = new_eye_origin - view_offset;

	new_player_angles = TransformAnglesToWorldSpace(player_angles, matrix.As3x4());
	new_player_angles.x = AngleNormalizePositive(new_player_angles.x);
	new_player_angles.y = AngleNormalizePositive(new_player_angles.y);
	new_player_angles.z = AngleNormalizePositive(new_player_angles.z);
}

CON_COMMAND(
    y_spt_calc_relative_position,
    "y_spt_calc_relative_position <index of the save glitch portal | \"blue\" | \"orange\"> [1 if you want to teleport there instead of just printing]")
{
	if (args.ArgC() != 2 && args.ArgC() != 3)
	{
		Msg("Usage: y_spt_calc_relative_position <index of the save glitch portal | \"blue\" | \"orange\"> [1 if you want to teleport there instead of just printing]\n");
		return;
	}

	if (_playerio.offServerAbsOrigin == 0)
	{
		Warning("Could not find the required offset in the client DLL.\n");
		return;
	}

	auto engine_server = GetEngine();
	auto engine = GetEngineClient();
	int portal_index = atoi(args.Arg(1));

	if (!strcmp(args.Arg(1), "blue") || !strcmp(args.Arg(1), "orange"))
	{
		std::vector<int> indices;

		for (int i = 0; i < MAX_EDICTS; ++i)
		{
			auto ent = engine_server->PEntityOfEntIndex(i);

			if (ent && !ent->IsFree() && !strcmp(ent->GetClassName(), "prop_portal"))
			{
				auto is_orange_portal =
				    *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(ent->GetUnknown()) + 1137);

				if (is_orange_portal == (args.Arg(1)[0] == 'o'))
					indices.push_back(i);
			}
		}

		if (indices.size() > 1)
		{
			Msg("There are multiple %s portals, please use the index:\n", args.Arg(1));

			for (auto i : indices)
			{
				auto ent = engine_server->PEntityOfEntIndex(i);
				auto& origin = *reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(ent->GetUnknown())
				                                          + _playerio.offServerAbsOrigin);

				Msg("%d located at %.8f %.8f %.8f\n", i, origin.x, origin.y, origin.z);
			}

			return;
		}
		else if (indices.size() == 0)
		{
			Msg("There are no %s portals.\n", args.Arg(1));
			return;
		}
		else
		{
			portal_index = indices[0];
		}
	}

	auto portal = engine_server->PEntityOfEntIndex(portal_index);
	if (!portal || portal->IsFree() || strcmp(portal->GetClassName(), "prop_portal") != 0)
	{
		Warning("The portal index is invalid.\n");
		return;
	}

	Vector new_player_origin;
	QAngle new_player_angles;
	calculate_offset_player_pos(portal, new_player_origin, new_player_angles);

	if (args.ArgC() == 2)
	{
		Msg("setpos %.8f %.8f %.8f;setang %.8f %.8f %.8f\n",
		    new_player_origin.x,
		    new_player_origin.y,
		    new_player_origin.z,
		    new_player_angles.x,
		    new_player_angles.y,
		    new_player_angles.z);
	}
	else
	{
		char buf[256];
		snprintf(buf,
		         ARRAYSIZE(buf),
		         "setpos %.8f %.8f %.8f;setang %.8f %.8f %.8f\n",
		         new_player_origin.x,
		         new_player_origin.y,
		         new_player_origin.z,
		         new_player_angles.x,
		         new_player_angles.y,
		         new_player_angles.z);

		engine->ClientCmd(buf);
	}
}

#endif

namespace playerio
{
	void SetTASInput(float* va, const Strafe::ProcessedFrame& out)
	{
		_playerio.SetTASInput(va, out);
	}

	void HandleAiming(float* va, bool& yawChanged)
	{
		_playerio.HandleAiming(va, yawChanged);
	}

	bool TryJump()
	{
		return _playerio.TryJump();
	}

	void Set_cinput_thisptr(void* thisptr)
	{
		cinput_thisptr = thisptr;
	}

	Strafe::PlayerData GetPlayerData()
	{
		return _playerio.GetPlayerData();
	}

	Strafe::MovementVars GetMovementVars()
	{
		return _playerio.GetMovementVars();
	}

	bool PlayerIOAddressesWereFound()
	{
		return _playerio.PlayerIOAddressesFound();
	}

	Vector GetPlayerVelocity()
	{
		return _playerio.GetPlayerVelocity();
	}

	Vector GetPlayerEyePos()
	{
		return _playerio.GetPlayerEyePos();
	}

	int playerio::GetPlayerFlags()
	{
		return _playerio.GetPlayerFlags();
	}

	bool playerio::GetFlagsDucking()
	{
		return _playerio.GetFlagsDucking();
	}

	bool playerio::IsGroundEntitySet()
	{
		return _playerio.IsGroundEntitySet();
	}
} // namespace playerio
