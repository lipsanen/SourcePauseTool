#include "stdafx.h"
#include "..\feature.hpp"
#include "..\OrangeBox\patterns.hpp"
#include "..\utils\game_detection.hpp"
#include "shadow.hpp"


typedef int(__fastcall* _GetShadowPosition)(void* thisptr, int edx, Vector* worldPosition, QAngle* angles);


class ShadowPosition : public Feature
{
public:
	static int __fastcall HOOKED_GetShadowPosition(void* thisptr, int edx, Vector* worldPosition, QAngle* angles);
	Vector PlayerHavokPos;

protected:
	_GetShadowPosition ORIG_GetShadowPosition;

	virtual bool ShouldLoadFeature() override
	{
#ifdef SSDK2013
		return !utils::DoesGameLookLikePortal();
#else

		return true;
#endif
	}

	virtual void InitHooks() override
	{
		HOOK_FUNCTION(vphysics, GetShadowPosition, ORIG_GetShadowPosition, ShadowPosition::HOOKED_GetShadowPosition);
	}

	virtual void LoadFeature() override
	{
	}

	virtual void UnloadFeature() override
	{
	}
};

static ShadowPosition g_Position;


int __fastcall ShadowPosition::HOOKED_GetShadowPosition(void* thisptr, int edx, Vector* worldPosition, QAngle* angles)
{
	int GetShadowPos = g_Position.ORIG_GetShadowPosition(thisptr, edx, worldPosition, angles);
	g_Position.PlayerHavokPos.x = worldPosition->x;
	g_Position.PlayerHavokPos.y = worldPosition->y;
	g_Position.PlayerHavokPos.z = worldPosition->z;
	return GetShadowPos;
}

Vector GetPlayerHavokPos()
{
	return g_Position.PlayerHavokPos;
}
