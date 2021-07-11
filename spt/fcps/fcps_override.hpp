#pragma once

#include "..\OrangeBox\spt-serverplugin.hpp"
#define GAME_DLL
#include "cbase.h"
#include "fcps_memory_repr.hpp"

// clang-format off

namespace fcps {

	// these were originally only used for the override but some are used in other spots
	namespace hacks {
		int tickCount();
		float curTime();
		int frameCount();
		const char* GetDebugName(CBaseEntity* pEntity);
	}

	enum FcpsCaller {
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
		// called when the m_bHeldObjectOnOppositeSideOfPortal flag switches from 1 to 0, only called on entities the player is holding
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


	enum FcpsCallResult {
		FCPS_Success,
		FCPS_Fail,
		FCPS_NotRun
	};


	extern char* FcpsCallerNames[];


	// regular fcps implemented in spt for debugging
	FcpsCallResult FcpsOverrideFunc(CBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask);

	FcpsCallResult FcpsOverrideFuncAndRecord(CBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask, FcpsCaller caller);
}
