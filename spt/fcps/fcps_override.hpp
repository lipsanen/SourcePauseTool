#pragma once

#include "..\OrangeBox\spt-serverplugin.hpp"

// clang-format off

namespace fcps {

	bool FindClosestPassableSpaceOverride(CBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask);

	static char* FcpsCallerNames[] = {"VPhysicsShadowUpdate", "TeleportTouchingEntity", "PortalSimulator::MoveTo", "RemoveEntityFromPortalHole", "CheckStuck", "Debug_FixMyPosition", "ReleaseAllEntityOwnership", "UNKNOWN"};

	enum FcpsCaller {
		//
		VPhysicsShadowUpdate,
		// when you're holding something
		TeleportTouchingEntity,
		PortalSimulator__MoveTo,
		RemoveEntityFromPortalHole,
		//
		CheckStuck,
		// command
		Debug_FixMyPosition,
		// when portal moves/closes?
		ReleaseAllEntityOwnership, // TODO - this is actually remove entity from portal hole
		Unknown
	};
}
