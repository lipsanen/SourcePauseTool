#pragma once

#include "..\OrangeBox\spt-serverplugin.hpp"
#define GAME_DLL
#include "cbase.h"

// clang-format off

namespace fcps {

	enum FcpsCaller : int {
		// happens when the player is in a portal environment and a bunch of other conditions
		// vIndecisivePush: newPosition - GetAbsOrigin()
		VPhysicsShadowUpdate,
		// inlined in CPortalSimulator::ReleaseAllEntityOwnership
		// called when the portal moves/closes?
		// vIndecisivePush: portal normal
		RemoveEntityFromPortalHole,
		// called when in portal hole and stuck on something?
		// vIndecisivePush: portal normal if in portal environment, <0,0,0> otherwise
		CheckStuck,
		// called when the m_bHeldObjectOnOppositeSideOfPortal flag switch from 1 to 0, only called on entities the player is holding
		// vIndecisivePush: portal normal of the portal to which the entity just teleported to?
		TeleportTouchingEntity,
		// not sure when this is called
		// vIndecisivePush: portal normal
		PortalSimulator__MoveTo,
		// called when running command debug_fixmyposition
		// vIndecisivePush: <0,0,0>
		Debug_FixMyPosition,
		Unknown
	};

	static char* FcpsCallerNames[] = {
		"CPortal_Player::VPhysicsShadowUpdate",
		"CPortalSimulator::RemoveEntityFromPortalHole",
		"CPortalGameMovement::CheckStuck",
		"CProp_Portal::TeleportTouchingEntity",
		"CPortalSimulator::MoveTo",
		"CC_Debug_FixMyPosition",
		"UNKNOWN"
	};

	// regular fcps implemented in spt for debugging
	bool FcpsOverride(CBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask);

	bool FcpsOverrideAndQueue(CBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask, FcpsCaller caller);
}
