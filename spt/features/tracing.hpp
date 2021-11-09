#pragma once
#include "..\feature.hpp"

#if defined(OE)
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif
#include "cmodel.h"

typedef void(__cdecl* _UTIL_TraceRay)(const Ray_t& ray,
                                      unsigned int mask,
                                      const IHandleEntity* ignore,
                                      int collisionGroup,
                                      trace_t* ptr);
typedef void(__cdecl* _TracePlayerBBoxForGround)(const Vector& start,
                                                 const Vector& end,
                                                 const Vector& mins,
                                                 const Vector& maxs,
                                                 IHandleEntity* player,
                                                 unsigned int fMask,
                                                 int collisionGroup,
                                                 trace_t& pm);
typedef void(__cdecl* _TracePlayerBBoxForGround2)(const Vector& start,
                                                  const Vector& end,
                                                  const Vector& mins,
                                                  const Vector& maxs,
                                                  IHandleEntity* player,
                                                  unsigned int fMask,
                                                  int collisionGroup,
                                                  trace_t& pm);
typedef void(__fastcall* _CGameMovement__TracePlayerBBox)(void* thisptr,
                                                          int edx,
                                                          const Vector& start,
                                                          const Vector& end,
                                                          unsigned int fMask,
                                                          int collisionGroup,
                                                          trace_t& pm);
typedef void(__fastcall* _CPortalGameMovement__TracePlayerBBox)(void* thisptr,
                                                                int edx,
                                                                const Vector& start,
                                                                const Vector& end,
                                                                unsigned int fMask,
                                                                int collisionGroup,
                                                                trace_t& pm);
typedef void(__fastcall* _SnapEyeAngles)(void* thisptr, int edx, const QAngle& viewAngles);
typedef float(__fastcall* _FirePortal)(void* thisptr, int edx, bool bPortal2, Vector* pVector, bool bTest);
typedef float(__fastcall* _TraceFirePortal)(void* thisptr,
                                            int edx,
                                            bool bPortal2,
                                            const Vector& vTraceStart,
                                            const Vector& vDirection,
                                            trace_t& tr,
                                            Vector& vFinalPosition,
                                            QAngle& qFinalAngles,
                                            int iPlacedBy,
                                            bool bTest);
typedef void*(__fastcall* _GetActiveWeapon)(void* thisptr);
typedef const Vector&(__fastcall* _CGameMovement__GetPlayerMaxs)(void* thisptr, int edx);
typedef const Vector&(__fastcall* _CGameMovement__GetPlayerMins)(void* thisptr, int edx);

// Tracing related functionality
class Tracing : public Feature
{
public:
	_FirePortal ORIG_FirePortal;
	_UTIL_TraceRay ORIG_UTIL_TraceRay;
	_TracePlayerBBoxForGround ORIG_TracePlayerBBoxForGround;
	_TracePlayerBBoxForGround2 ORIG_TracePlayerBBoxForGround2;
	_SnapEyeAngles ORIG_SnapEyeAngles;
	trace_t lastPortalTrace;
	_GetActiveWeapon ORIG_GetActiveWeapon;

	bool TraceClientRay(const Ray_t& ray,
	                    unsigned int mask,
	                    const IHandleEntity* ignore,
	                    int collisionGroup,
	                    trace_t* ptr);
	bool CanTracePlayerBBox();
	void TracePlayerBBox(const Vector& start,
	                     const Vector& end,
	                     const Vector& mins,
	                     const Vector& maxs,
	                     unsigned int fMask,
	                     int collisionGroup,
	                     trace_t& pm);
	float TraceFirePortal(trace_t& tr, const Vector& startPos, const Vector& vDirection);

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	static float __fastcall HOOKED_TraceFirePortal(void* thisptr,
	                                               int edx,
	                                               bool bPortal2,
	                                               const Vector& vTraceStart,
	                                               const Vector& vDirection,
	                                               trace_t& tr,
	                                               Vector& vFinalPosition,
	                                               QAngle& qFinalAngles,
	                                               int iPlacedBy,
	                                               bool bTest);
	static const Vector& __fastcall HOOKED_CGameMovement__GetPlayerMaxs(void* thisptr, int edx);
	static const Vector& __fastcall HOOKED_CGameMovement__GetPlayerMins(void* thisptr, int edx);

	bool overrideMinMax;
	Vector _mins;
	Vector _maxs;

	_CGameMovement__TracePlayerBBox ORIG_CGameMovement__TracePlayerBBox;
	_CPortalGameMovement__TracePlayerBBox ORIG_CPortalGameMovement__TracePlayerBBox;
	_CGameMovement__GetPlayerMins ORIG_CGameMovement__GetPlayerMins;
	_CGameMovement__GetPlayerMaxs ORIG_CGameMovement__GetPlayerMaxs;
	_TraceFirePortal ORIG_TraceFirePortal;
};

extern Tracing g_Tracing;
