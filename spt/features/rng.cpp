#include "stdafx.h"
#include "rng.hpp"
#include "tier1/checksum_md5.h"
#include "cmodel.h"

#ifdef OE
#include "..\game_shared\usercmd.h"
#else
#include "usercmd.h"
#endif

RNGStuff _rng;

int RNGStuff::GetPredictionRandomSeed(int commandOffset)
{
	int command_number = _rng.commandNumber + commandOffset;
	return MD5_PseudoRandom(command_number) & 0x7fffffff;
}

bool RNGStuff::ShouldLoadFeature()
{
	return true;
}

void RNGStuff::InitHooks() 
{
	HOOK_FUNCTION(server, SetPredictionRandomSeed);
}

void RNGStuff::LoadFeature() {}

void RNGStuff::UnloadFeature() {}

void __cdecl RNGStuff::HOOKED_SetPredictionRandomSeed(void* usercmd)
{
	CUserCmd* ptr = reinterpret_cast<CUserCmd*>(usercmd);
	if (ptr)
	{
		_rng.commandNumber = ptr->command_number;
	}

	_rng.ORIG_SetPredictionRandomSeed(usercmd);
}
