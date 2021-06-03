#include "stdafx.h"

#include "fcps_override.hpp"

#include "..\OrangeBox\modules.hpp"
#define GAME_DLL
#include "cbase.h"


// clang-format off

namespace fcps {

	// implementations for non-virtual and non-inlined methods
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

		// sdk doesn't have the correct number of virtual functions so the wrong one gets called
		inline void Teleport(CBaseEntity* pEntity, const Vector* newPosition, const QAngle* newAngles, const Vector* newVelocity) {
			__asm {
				push newVelocity
				push newAngles
				push newPosition
				mov ecx, dword ptr [pEntity]
				mov edx, dword ptr [ecx]
				mov eax, dword ptr [edx+0x1a4]
				call eax
			}
		}

		// sdk 'm_Collision' field is off by 4 bytes
		inline CCollisionProperty* CollisionProp(CBaseEntity* pEntity) {
			return (CCollisionProperty*)((uint32_t)pEntity + 320);
		}
	}


	// basically an exact ripoff of the regular fcps
	bool FindClosestPassableSpaceOverride(CBaseEntity* pEntity, const Vector& vIndecisivePush, unsigned int fMask) {

		DevMsg("spt: Running override of FCPS\n");

		if (!g_pCVar->FindVar("sv_use_find_closest_passable_space")->GetBool())
			return true;

		if (!hacks::CanDoHacks()) {
			Msg("spt: Cannot run custom fcps, one or more functions were not found\n");
			return true;
		}

		// TODO figure out where this was called from


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
}
