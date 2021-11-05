#include "stdafx.h"
#include "afterframes.hpp"
#include "generic.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\OrangeBox\cvars.hpp"
#include "dbg.h"
#include "..\OrangeBox\spt-serverplugin.hpp"
#include <sstream>

AfterframesFeature _afterframes;

ConVar _y_spt_afterframes_await_legacy("_y_spt_afterframes_await_legacy",
	"0",
	FCVAR_TAS_RESET,
	"Set to 1 for backwards compatibility with old scripts.");
ConVar _y_spt_afterframes_reset_on_server_activate("_y_spt_afterframes_reset_on_server_activate", "1", FCVAR_ARCHIVE);


void AfterframesFeature::AddAfterFramesEntry(afterframes_entry_t entry)
{
	afterframesQueue.push_back(entry);
}

void AfterframesFeature::DelayAfterframesQueue(int delay)
{
	afterframesDelay = delay;
}

void AfterframesFeature::ResetAfterframesQueue()
{
	afterframesQueue.clear();
}

void AfterframesFeature::PauseAfterframesQueue()
{
	afterframesPaused = true;
}

void AfterframesFeature::ResumeAfterframesQueue()
{
	afterframesPaused = false;
}

void AfterframesFeature::SV_ActivateServer()
{
	if (_y_spt_afterframes_reset_on_server_activate.GetBool())
		ResetAfterframesQueue();
}

void AfterframesFeature::FinishRestore()
{
	if (_y_spt_afterframes_await_legacy.GetBool())
		ResumeAfterframesQueue();
}

void AfterframesFeature::SetPaused(bool paused)
{
	if (!paused && !_y_spt_afterframes_await_legacy.GetBool())
		ResumeAfterframesQueue();
}

bool AfterframesFeature::ShouldLoadFeature()
{
	return true;
}

void AfterframesFeature::InitHooks()
{
}

void AfterframesFeature::LoadFeature()
{
	if (!generic_.ORIG_HudUpdate)
	{
		Warning("_y_spt_afterframes has no effect.\n");
	}
	else
	{
		generic_.FrameSignal.Connect(this, &AfterframesFeature::OnFrame);
		afterframesPaused = false;
		afterframesDelay = 0;
		afterframesQueue.clear();
	}
}

void AfterframesFeature::UnloadFeature()
{
	afterframesQueue.clear();
}

void AfterframesFeature::OnFrame()
{
	if (afterframesPaused)
	{
		return;
	}

	if (afterframesDelay > 0)
	{
		--afterframesDelay;
		return;
	}

	for (auto it = afterframesQueue.begin(); it != afterframesQueue.end();)
	{
		it->framesLeft--;
		if (it->framesLeft <= 0)
		{
			EngineConCmd(it->command.c_str());
			it = afterframesQueue.erase(it);
		}
		else
			++it;
	}

	AfterFramesSignal();
}

CON_COMMAND(_y_spt_afterframes_wait, "Delays the afterframes queue. Usage: _y_spt_afterframes_wait <delay>")
{
#if defined(OE)
	auto engine = GetEngineClient();
	if (!engine)
		return;
	ArgsWrapper args(engine);
#endif

	if (args.ArgC() != 2)
	{
		Msg("Usage: _y_spt_afterframes_wait <delay>\n");
		return;
	}

	int delay = std::stoi(args.Arg(1));

	_afterframes.DelayAfterframesQueue(delay);
}

CON_COMMAND(_y_spt_afterframes, "Add a command into an afterframes queue. Usage: _y_spt_afterframes <count> <command>")
{
#if defined(OE)
	auto engine = GetEngineClient();
	if (!engine)
		return;
	ArgsWrapper args(engine);
#endif

	if (args.ArgC() != 3)
	{
		Msg("Usage: _y_spt_afterframes <count> <command>\n");
		return;
	}

	afterframes_entry_t entry;

	std::istringstream ss(args.Arg(1));
	ss >> entry.framesLeft;
	entry.command.assign(args.Arg(2));

	_afterframes.AddAfterFramesEntry(entry);
}

#if !defined(OE)
CON_COMMAND(
	_y_spt_afterframes2,
	"Add everything after count as a command into the queue. Do not insert the command in quotes. Usage: _y_spt_afterframes2 <count> <command>")
{
	if (args.ArgC() < 3)
	{
		Msg("Usage: _y_spt_afterframes2 <count> <command>\n");
		return;
	}

	afterframes_entry_t entry;

	std::istringstream ss(args.Arg(1));
	ss >> entry.framesLeft;
	const char* cmd = args.ArgS() + strlen(args.Arg(1)) + 1;
	entry.command.assign(cmd);

	_afterframes.AddAfterFramesEntry(entry);
}
#endif

CON_COMMAND(
	_y_spt_afterframes_await_load,
	"Pause reading from the afterframes queue until the next load or changelevel. Useful for writing scripts spanning multiple maps or save-load segments.")
{
	_afterframes.PauseAfterframesQueue();
}

CON_COMMAND(_y_spt_afterframes_reset, "Reset the afterframes queue.")
{
	_afterframes.ResetAfterframesQueue();
}