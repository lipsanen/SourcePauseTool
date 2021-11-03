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
}

void GenericFeature::LoadFeature()
{
}

void GenericFeature::UnloadFeature()
{
}
