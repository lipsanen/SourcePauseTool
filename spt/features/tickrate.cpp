#include "stdafx.h"
#include "tickrate.hpp"
#include "convar.h"
#include "dbg.h"

TickrateMod _tickrate;

float TickrateMod::GetTickrate()
{
	if (pIntervalPerTick)
		return *pIntervalPerTick;

	return 0.0f;
}

void TickrateMod::SetTickrate(float tickrate)
{
	if (pIntervalPerTick)
		*pIntervalPerTick = tickrate;
}

bool TickrateMod::ShouldLoadFeature()
{
	return true;
}

void TickrateMod::InitHooks()
{
	FIND_PATTERN(engine, MiddleOfSV_InitGameDLL);
}

void TickrateMod::LoadFeature()
{
	// interval_per_tick
	if (ORIG_MiddleOfSV_InitGameDLL)
	{
		int ptnNumber = GetPatternIndex((PVOID*)&ORIG_MiddleOfSV_InitGameDLL);

		switch (ptnNumber)
		{
		case 0:
			pIntervalPerTick = *reinterpret_cast<float**>(ORIG_MiddleOfSV_InitGameDLL + 18);
			break;

		case 1:
			pIntervalPerTick = *reinterpret_cast<float**>(ORIG_MiddleOfSV_InitGameDLL + 16);
			break;
		default:
			pIntervalPerTick = nullptr;
			break;
		}

		DevMsg("Found interval_per_tick at %p.\n", pIntervalPerTick);
	}
	else
	{
		Warning("_y_spt_tickrate has no effect.\n");
		pIntervalPerTick = nullptr;
	}
}

void TickrateMod::UnloadFeature() {}

CON_COMMAND(_y_spt_tickrate, "Get or set the tickrate. Usage: _y_spt_tickrate [tickrate]")
{
#if defined(OE)
	if (!engine)
		return;

	ArgsWrapper args(engine.get());
#endif

	switch (args.ArgC())
	{
	case 1:
		Msg("Current tickrate: %f\n", _tickrate.GetTickrate());
		break;

	case 2:
		_tickrate.SetTickrate(atof(args.Arg(1)));
		break;

	default:
		Msg("Usage: _y_spt_tickrate [tickrate]\n");
	}
}