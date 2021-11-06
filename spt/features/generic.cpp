#include "stdafx.h"

#include "generic.hpp"
#include "playerio.hpp"
#include "..\utils\ent_utils.hpp"
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
}

void GenericFeature::LoadFeature()
{

}

void GenericFeature::UnloadFeature()
{
}

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
