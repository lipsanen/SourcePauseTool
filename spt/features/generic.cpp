#include "stdafx.h"

#include "generic.hpp"
#include "playerio.hpp"
#include "..\utils\ent_utils.hpp"
#include "..\utils\game_detection.hpp"
#include "convar.h"

GenericFeature generic_;

void GenericFeature::Tick()
{
	if (utils::playerEntityAvailable())
	{
		OngroundSignal(playerio::IsGroundEntitySet());
		TickSignal();
	}
}

Vector GenericFeature::GetCameraOrigin()
{
	if (ORIG_MainViewOrigin)
		return ORIG_MainViewOrigin();
	else
		return Vector(0);
}

bool GenericFeature::ShouldLoadFeature()
{
	return true;
}

void GenericFeature::InitHooks()
{
	HOOK_FUNCTION(client, HudUpdate);
	HOOK_FUNCTION(engine, FinishRestore);
	HOOK_FUNCTION(engine, SetPaused);
	HOOK_FUNCTION(engine, SV_ActivateServer);
	FIND_PATTERN(engine, CEngineTrace__PointOutsideWorld);
	FIND_PATTERN(client, CHudDamageIndicator__GetDamagePosition);
}

void GenericFeature::LoadFeature()
{
	if (ORIG_CHudDamageIndicator__GetDamagePosition)
	{
		int offset = *reinterpret_cast<int*>(ORIG_CHudDamageIndicator__GetDamagePosition + 4);
		ORIG_MainViewOrigin = (_MainViewOrigin)(offset + ORIG_CHudDamageIndicator__GetDamagePosition + 8);
		DevMsg("[client.dll] Found MainViewOrigin at %p\n", ORIG_MainViewOrigin);
	}

	if (ORIG_CHLClient__CanRecordDemo)
	{
		int offset = *reinterpret_cast<int*>(ORIG_CHLClient__CanRecordDemo + 1);
		ORIG_GetClientModeNormal = (_GetClientModeNormal)(offset + ORIG_CHLClient__CanRecordDemo + 5);
		DevMsg("[client.dll] Found GetClientModeNormal at %p\n", ORIG_GetClientModeNormal);
	}

	if (!ORIG_MainViewOrigin || !ORIG_CEngineTrace__PointOutsideWorld)
		Warning("y_spt_hud_oob 1 has no effect\n");
}

void GenericFeature::UnloadFeature() {}

void __stdcall GenericFeature::HOOKED_HudUpdate(bool bActive)
{
	generic_.FrameSignal();
	generic_.ORIG_HudUpdate(bActive);
}

bool __cdecl GenericFeature::HOOKED_SV_ActivateServer()
{
	bool result = generic_.ORIG_SV_ActivateServer();
	generic_.SV_ActivateServerSignal(result);

	return result;
}

void __fastcall GenericFeature::HOOKED_FinishRestore(void* thisptr, int edx)
{
	generic_.ORIG_FinishRestore(thisptr, edx);
	generic_.FinishRestoreSignal(thisptr, edx);
}

void __fastcall GenericFeature::HOOKED_SetPaused(void* thisptr, int edx, bool paused)
{
	generic_.SetPausedSignal(thisptr, edx, paused);

	if (paused == false)
	{
		if (generic_.shouldPreventNextUnpause)
		{
			DevMsg("Unpause prevented.\n");
			generic_.shouldPreventNextUnpause = false;
			return;
		}
	}

	return generic_.ORIG_SetPaused(thisptr, edx, paused);
}
