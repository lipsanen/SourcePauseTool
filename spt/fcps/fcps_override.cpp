#include "stdafx.h"

#include "fcps_override.hpp"

#include "..\OrangeBox\modules.hpp"
#include "..\OrangeBox\spt-serverplugin.hpp"

// clang-format off

namespace fcps {

	
	char* FcpsCallerNames[] = {
		"CPortal_Player::VPhysicsShadowUpdate",
		"CPortalSimulator::RemoveEntityFromPortalHole",
		"CPortalGameMovement::CheckStuck",
		"CProp_Portal::TeleportTouchingEntity",
		"CPortalSimulator::MoveTo",
		"CC_Debug_FixMyPosition",
		"UNKNOWN"
	};


	// implementations for non-virtual and non-inlined methods, or for functions/fields which are at the wrong offset in the sdk due to an incorrect number of fields
	namespace hacks {

		bool CanDoHacks() {
			return serverDLL.ORIG_UTIL_TraceRay && engineDLL.ORIG_CEngineTrace__PointOutsideWorld;
		}

		inline void UTIL_TraceRay(const Ray_t& ray, unsigned int mask, const IHandleEntity* ignore, int collisionGroup, trace_t* ptr) {
			serverDLL.ORIG_UTIL_TraceRay(ray, mask, ignore, collisionGroup, ptr);
		}

		inline bool CEngineTrace__PointOutsideWorld(const Vector& pt) {
			return engineDLL.ORIG_CEngineTrace__PointOutsideWorld(nullptr, 0, pt);
		}

		inline const Vector& GetAbsOrigin(CBaseEntity* pEntity) {
			if (pEntity->IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
				Warning("spt: entity has dirty abs transform during fcps\n");
			return *(Vector*)((uint32_t)pEntity + 580);
		}

		inline const QAngle& GetAbsAngles(CBaseEntity* pEntity) {
			if (pEntity->IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
				Warning("spt: entity has dirty abs transform during fcps\n");
			return *(QAngle*)((uint32_t)pEntity + 704);
		}

		void CollisionAABBToWorldAABB(CCollisionProperty* pEntityCollision, const Vector& entityMins, const Vector& entityMaxs, Vector* pWorldMins, Vector* pWorldMaxs) {
			if (!pEntityCollision->IsBoundsDefinedInEntitySpace() || (pEntityCollision->GetCollisionAngles() == vec3_angle)) {
				VectorAdd(entityMins, pEntityCollision->GetCollisionOrigin(), *pWorldMins);
				VectorAdd(entityMaxs, pEntityCollision->GetCollisionOrigin(), *pWorldMaxs);
			} else {
				TransformAABB(pEntityCollision->CollisionToWorldTransform(), entityMins, entityMaxs, *pWorldMins, *pWorldMaxs);
			}
		}

		inline void WorldSpaceAABB(CCollisionProperty* pEntityCollision, Vector* pWorldMins, Vector* pWorldMaxs) {
			CollisionAABBToWorldAABB(pEntityCollision, pEntityCollision->OBBMins(), pEntityCollision->OBBMaxs(), pWorldMins, pWorldMaxs);
		}

		inline bool HasMoveParent(CBaseEntity* pEntity) {
			EHANDLE m_hMoveParent = *(EHANDLE*)((uint32_t)pEntity + 308);
			return m_hMoveParent.ToInt() != -1;
		}

		#define VIRTUAL_CALL(obj, byteOffset) __asm { \
			__asm mov ecx, dword ptr [obj] \
			__asm mov edx, dword ptr [ecx] \
			__asm mov eax, dword ptr [edx + byteOffset] \
			__asm call eax \
		}

		inline void __cdecl Teleport(CBaseEntity* pEntity, const Vector* newPosition, const QAngle* newAngles, const Vector* newVelocity) {
			__asm {
				push newVelocity
				push newAngles
				push newPosition
			}
			VIRTUAL_CALL(pEntity, 0x1a4)
		}

		inline unsigned short __cdecl GetGameFlags(IPhysicsObject* pPhys) {
			VIRTUAL_CALL(pPhys, 0x4c)
		}

		inline bool __cdecl IsPlayer(CBaseEntity* pEntity) {
			VIRTUAL_CALL(pEntity, 0x138)
		}

		inline CCollisionProperty* CollisionProp(CBaseEntity* pEntity) {
			return (CCollisionProperty*)((uint32_t)pEntity + 320);
		}

		inline IPhysicsObject* VPhysicsGetObject(CBaseEntity* pEntity) {
			return *(IPhysicsObject**)((uint32_t)pEntity + 0x1a8);
		}

		inline int tickCount() {
			return *((int*)engineDLL.pGameServer + 3);
		}

		inline char* mapName() {
			return (char*)engineDLL.pGameServer + 16;
		}

		inline float curTime() {
			return *((float*)engineDLL.pGameServer + 66) * tickCount();
		}
	}


	// a ripoff of the regular fcps for debugging
	bool FcpsOverride(CBaseEntity* pEntity, const Vector& vIndecisivePush, unsigned int fMask) {

		DevMsg("spt: Running override of FCPS\n");

		if (!g_pCVar->FindVar("sv_use_find_closest_passable_space")->GetBool())
			return true;

		if (!hacks::CanDoHacks()) {
			Warning("spt: Cannot run custom FCPS, one or more necessary functions were not found\n");
			return true;
		}

		if (hacks::HasMoveParent(pEntity))
			return true;

		Vector ptExtents[8]; // ordering is going to be like 3 bits, where 0 is a min on the related axis, and 1 is a max on the same axis, axis order x y z
		float fExtentsValidation[8]; // some points are more valid than others, and this is our measure

		Vector vEntityMaxs;
		Vector vEntityMins;
		CCollisionProperty* pEntityCollision = hacks::CollisionProp(pEntity);
		hacks::WorldSpaceAABB(pEntityCollision, &vEntityMins, &vEntityMaxs);

		Vector ptEntityCenter = (vEntityMins + vEntityMaxs) / 2.0f;
		vEntityMins -= ptEntityCenter;
		vEntityMaxs -= ptEntityCenter;

		Vector ptEntityOriginalCenter = ptEntityCenter;
		ptEntityCenter.z += 0.001f; // to satisfy m_IsSwept on first pass
		int iEntityCollisionGroup = pEntity->GetCollisionGroup();

		trace_t traces[2];
		Ray_t entRay;
		entRay.m_Extents = vEntityMaxs;
		entRay.m_IsRay = false;
		entRay.m_IsSwept = true;
		entRay.m_StartOffset = vec3_origin;

		Vector vOriginalExtents = vEntityMaxs;
		Vector vGrowSize = vEntityMaxs / 101.0f;
		vEntityMaxs -= vGrowSize;
		vEntityMins += vGrowSize;
		
		Ray_t testRay;
		testRay.m_Extents = vGrowSize;
		testRay.m_IsRay = false;
		testRay.m_IsSwept = true;
		testRay.m_StartOffset = vec3_origin;

		for(unsigned int iFailCount = 0; iFailCount != 100; ++iFailCount) {

			entRay.m_Start = ptEntityCenter;
			entRay.m_Delta = ptEntityOriginalCenter - ptEntityCenter;

			hacks::UTIL_TraceRay(entRay, fMask, pEntity, iEntityCollisionGroup, &traces[0]);
			if( traces[0].startsolid == false ) {
				Vector vNewPos = traces[0].endpos + (hacks::GetAbsOrigin(pEntity) - ptEntityOriginalCenter);
				hacks::Teleport(pEntity, &vNewPos, nullptr, nullptr);
				return true; // current placement worked
			}

			bool bExtentInvalid[8];
			for(int i = 0; i != 8; ++i) {
				fExtentsValidation[i] = 0.0f;
				ptExtents[i] = ptEntityCenter;
				ptExtents[i].x += i & (1 << 0) ? vEntityMaxs.x : vEntityMins.x;
				ptExtents[i].y += i & (1 << 1) ? vEntityMaxs.y : vEntityMins.y;
				ptExtents[i].z += i & (1 << 2) ? vEntityMaxs.z : vEntityMins.z;
				bExtentInvalid[i] = hacks::CEngineTrace__PointOutsideWorld(ptExtents[i]);
			}

			for(unsigned int counter = 0; counter != 7; ++counter) {
				for(unsigned int counter2 = counter + 1; counter2 != 8; ++counter2) {

					testRay.m_Delta = ptExtents[counter2] - ptExtents[counter];
					if(bExtentInvalid[counter]) {
						traces[0].startsolid = true;
					} else {
						testRay.m_Start = ptExtents[counter];
						hacks::UTIL_TraceRay(testRay, fMask, pEntity, iEntityCollisionGroup, &traces[0]);
					}

					if(bExtentInvalid[counter2]) {
						traces[1].startsolid = true;
					} else {
						testRay.m_Start = ptExtents[counter2];
						testRay.m_Delta = -testRay.m_Delta;
						hacks::UTIL_TraceRay(testRay, fMask, pEntity, iEntityCollisionGroup, &traces[1]);
					}

					float fDistance = testRay.m_Delta.Length();

					for(int i = 0; i != 2; ++i) {
						int iExtent = i == 0 ? counter : counter2;
						if( traces[i].startsolid )
							fExtentsValidation[iExtent] -= 100.0f;
						else
							fExtentsValidation[iExtent] += traces[i].fraction * fDistance;
					}
				}
			}

			Vector vNewOriginDirection( 0.0f, 0.0f, 0.0f );
			float fTotalValidation = 0.0f;
			for(unsigned int counter = 0; counter != 8; ++counter) {
				if(fExtentsValidation[counter] > 0.0f) {
					vNewOriginDirection += (ptExtents[counter] - ptEntityCenter) * fExtentsValidation[counter];
					fTotalValidation += fExtentsValidation[counter];
				}
			}

			if(fTotalValidation != 0.0f) {
				ptEntityCenter += vNewOriginDirection / fTotalValidation;
				// increase sizing
				testRay.m_Extents += vGrowSize;
				vEntityMaxs -= vGrowSize;
				vEntityMins = -vEntityMaxs;
			} else {
				// no point was valid, apply the indecisive vector & reset sizing
				ptEntityCenter += vIndecisivePush;
				testRay.m_Extents = vGrowSize;
				vEntityMaxs = vOriginalExtents;
				vEntityMins = -vEntityMaxs;
			}		
		}
		return false;
	}


	// in this version we record the process of the algorithm as an fcps event
	FcpsEvent* FcpsOverrideAndRecord(CBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask, FcpsCaller caller) {

		// TODO - vww in this version and the non-record kills you

		DevMsg("spt: Running override of FCPS\n");

		if (!g_pCVar->FindVar("sv_use_find_closest_passable_space")->GetBool())
			return nullptr;

		if (!hacks::CanDoHacks()) {
			Warning("spt: Cannot run custom FCPS, one or more necessary functions were not found\n");
			return nullptr;
		}

		if (hacks::HasMoveParent(pEntity))
			return nullptr;

		// init new event and fill it with data as the alg iterates
		FcpsEvent& thisEvent = RecordedFcpsQueue->beginNextEvent();

		// general info
		strcpy_s(thisEvent.mapName, MAP_NAME_LEN, hacks::mapName());
		thisEvent.caller = caller;
		thisEvent.tickTime = hacks::tickCount();
		thisEvent.curTime = hacks::curTime();
		thisEvent.wasRunOnPlayer = hacks::IsPlayer(pEntity);
		strcpy_s(thisEvent.entClassName, ENT_CLASS_NAME_LEN, pEntity->GetClassname());
		IPhysicsObject *pPhys = hacks::VPhysicsGetObject(pEntity);
		if (pPhys && hacks::GetGameFlags(pPhys) & FVPHYSICS_PLAYER_HELD) {
			thisEvent.isHeldObject = true;
			CBaseEntity* holdingPlayer = (CBaseEntity*)GetServerPlayer();
			CCollisionProperty* pPlayerCollision = hacks::CollisionProp(holdingPlayer);
			hacks::WorldSpaceAABB(pPlayerCollision, &thisEvent.playerMins, &thisEvent.playerMaxs);
		} else {
			thisEvent.isHeldObject = false;
		}
		thisEvent.fMask = fMask;


		Vector vEntityMaxs;
		Vector vEntityMins;
		CCollisionProperty* pEntityCollision = hacks::CollisionProp(pEntity);
		hacks::WorldSpaceAABB(pEntityCollision, &vEntityMins, &vEntityMaxs);

		Vector ptEntityCenter = (vEntityMins + vEntityMaxs) / 2.0f;
		vEntityMins -= ptEntityCenter;
		vEntityMaxs -= ptEntityCenter;
		thisEvent.entMins = vEntityMins;
		thisEvent.entMaxs = vEntityMaxs;
		thisEvent.originalCenter = ptEntityCenter;

		Vector ptExtents[8]; // ordering is going to be like 3 bits, where 0 is a min on the related axis, and 1 is a max on the same axis, axis order x y z
		float fExtentsValidation[8]; // some points are more valid than others, and this is our measure

		Vector ptEntityOriginalCenter = ptEntityCenter;
		ptEntityCenter.z += 0.001f; // to satisfy m_IsSwept on first pass
		thisEvent.zNudgedCenter = ptEntityCenter;
		int iEntityCollisionGroup = pEntity->GetCollisionGroup();
		thisEvent.collisionGroup = iEntityCollisionGroup;

		trace_t traces[2];
		Ray_t entRay;
		entRay.m_Extents = vEntityMaxs;
		entRay.m_IsRay = false;
		entRay.m_IsSwept = true;
		entRay.m_StartOffset = vec3_origin;

		Vector vOriginalExtents = vEntityMaxs;	
		Vector vGrowSize = vEntityMaxs / 101.0f;
		vEntityMaxs -= vGrowSize;
		vEntityMins += vGrowSize;

		thisEvent.growSize = vGrowSize;
		thisEvent.adjustedMins = vEntityMins;
		thisEvent.adjustedMaxs = vEntityMaxs;
		thisEvent.loopStartCount = thisEvent.loopFinishCount = 0;
		
		Ray_t testRay;
		testRay.m_Extents = vGrowSize;
		testRay.m_IsRay = false;
		testRay.m_IsSwept = true;
		testRay.m_StartOffset = vec3_origin;


		for(unsigned int iFailCount = 0; iFailCount != 100; ++iFailCount) {

			thisEvent.loopStartCount++;

			entRay.m_Start = ptEntityCenter;
			entRay.m_Delta = ptEntityOriginalCenter - ptEntityCenter;

			hacks::UTIL_TraceRay(entRay, fMask, pEntity, iEntityCollisionGroup, &traces[0]);

			FcpsEvent::FcpsLoop& thisLoop = thisEvent.loops[iFailCount];
			thisLoop.failCount = iFailCount;
			thisLoop.testRay = entRay;
			thisLoop.testTraceResult = traces[0];

			if( traces[0].startsolid == false ) {
				Vector vNewPos = traces[0].endpos + (hacks::GetAbsOrigin(pEntity) - ptEntityOriginalCenter);
				hacks::Teleport(pEntity, &vNewPos, nullptr, nullptr);
				thisEvent.wasSuccess = true; // current placement worked
				thisEvent.newPos = vNewPos;
				return &thisEvent;
			}

			bool bExtentInvalid[8];
			for(int i = 0; i != 8; ++i) {
				fExtentsValidation[i] = 0.0f;
				ptExtents[i] = ptEntityCenter;
				ptExtents[i].x += i & (1 << 0) ? vEntityMaxs.x : vEntityMins.x;
				ptExtents[i].y += i & (1 << 1) ? vEntityMaxs.y : vEntityMins.y;
				ptExtents[i].z += i & (1 << 2) ? vEntityMaxs.z : vEntityMins.z;
				bExtentInvalid[i] = hacks::CEngineTrace__PointOutsideWorld(ptExtents[i]);

				thisLoop.corners[i] = ptExtents[i];
				thisLoop.cornersOob[i] = bExtentInvalid[i];
			}
			thisLoop.validationRayCheckCount = 0;

			for(unsigned int counter = 0; counter != 7; ++counter) {
				for(unsigned int counter2 = counter + 1; counter2 != 8; ++counter2) {

					auto& thisValidationCheck = thisLoop.validationRayChecks[thisLoop.validationRayCheckCount++]; // this'll just go up to its max
					thisValidationCheck.cornerIdx[0] = counter;
					thisValidationCheck.cornerIdx[1] = counter2;

					testRay.m_Delta = ptExtents[counter2] - ptExtents[counter];
					if(bExtentInvalid[counter]) {
						traces[0].startsolid = true;
					} else {
						testRay.m_Start = ptExtents[counter];
						hacks::UTIL_TraceRay(testRay, fMask, pEntity, iEntityCollisionGroup, &traces[0]);
						thisValidationCheck.ray[0] = testRay;
						thisValidationCheck.trace[0] = traces[0];
					}

					if(bExtentInvalid[counter2]) {
						traces[1].startsolid = true;
					} else {
						testRay.m_Start = ptExtents[counter2];
						testRay.m_Delta = -testRay.m_Delta;
						hacks::UTIL_TraceRay(testRay, fMask, pEntity, iEntityCollisionGroup, &traces[1]);
						thisValidationCheck.ray[1] = testRay;
						thisValidationCheck.trace[1] = traces[1];
					}

					float fDistance = testRay.m_Delta.Length();

					for(int i = 0; i != 2; ++i) {
						int iExtent = i == 0 ? counter : counter2;
						float validationDelta = traces[i].startsolid ? -100.0f : traces[i].fraction * fDistance;
						fExtentsValidation[iExtent] += validationDelta;
						thisValidationCheck.validationDelta[i] = validationDelta;
					}
				}
			}

			Vector vNewOriginDirection( 0.0f, 0.0f, 0.0f );
			float fTotalValidation = 0.0f;
			for(unsigned int counter = 0; counter != 8; ++counter) {
				if(fExtentsValidation[counter] > 0.0f) {
					vNewOriginDirection += (ptExtents[counter] - ptEntityCenter) * fExtentsValidation[counter];
					fTotalValidation += fExtentsValidation[counter];
				}
				thisLoop.cornerValidation[counter] = fExtentsValidation[counter];
			}
			thisLoop.totalValidation = fTotalValidation;

			if(fTotalValidation != 0.0f) {
				ptEntityCenter += thisLoop.newOriginDirection = vNewOriginDirection / fTotalValidation;
				// increase sizing
				testRay.m_Extents += vGrowSize;
				vEntityMaxs -= vGrowSize;
				vEntityMins = -vEntityMaxs;
			} else {
				// no point was valid, apply the indecisive vector & reset sizing
				ptEntityCenter += thisLoop.newOriginDirection = vIndecisivePush;
				testRay.m_Extents = vGrowSize;
				vEntityMaxs = vOriginalExtents;
				vEntityMins = -vEntityMaxs;
			}
			thisLoop.newCenter = ptEntityCenter;
			thisLoop.newMins = vEntityMins;
			thisLoop.newMaxs = vEntityMaxs;
			thisEvent.loopFinishCount++;
		}
		thisEvent.wasSuccess = false;
		return &thisEvent;
	}
}
