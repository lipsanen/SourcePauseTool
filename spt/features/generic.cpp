#include "stdafx.h"

#include "generic.hpp"
#include "playerio.hpp"
#include "..\utils\ent_utils.hpp"

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
