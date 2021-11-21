#include "stdafx.h"
#include "..\feature.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\utils\game_detection.hpp"
#include "afterframes.hpp"
#include "convar.h"
#include "dbg.h"

typedef void(__fastcall* _)(void* thisptr, int edx, void* a, void* b);
typedef void(__fastcall* _SlidingAndOtherStuff)(void* thisptr, int edx, void* a, void* b);

ConVar y_spt_on_slide_pause_for("y_spt_on_slide_pause_for",
                                "0",
                                0,
                                "Whenever sliding occurs in DMoMM, pause for this many ticks.");

// DMoMM stuff
class DMoMM : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void PreHook() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	void* ORIG_MiddleOfSlidingFunction;
	_SlidingAndOtherStuff ORIG_SlidingAndOtherStuff;
	bool sliding;
	bool wasSliding;
	static void __fastcall HOOKED_SlidingAndOtherStuff(void* thisptr, int edx, void* a, void* b);
	static void HOOKED_MiddleOfSlidingFunction();
	void HOOKED_MiddleOfSlidingFunction_Func();
};

static DMoMM _dmomm;

bool DMoMM::ShouldLoadFeature()
{
	return utils::DoesGameLookLikeDMoMM();
}

void DMoMM::InitHooks()
{
	HOOK_FUNCTION(server, MiddleOfSlidingFunction);
}

void DMoMM::PreHook()
{
	// Middle of DMoMM sliding function.
	if (ORIG_MiddleOfSlidingFunction)
	{
		ORIG_SlidingAndOtherStuff = (_SlidingAndOtherStuff)(&ORIG_MiddleOfSlidingFunction - 0x4bb);
		ADD_RAW_HOOK(server, SlidingAndOtherStuff);
		DevMsg("[server.dll] Found the sliding function at %p.\n", ORIG_SlidingAndOtherStuff);
	}
	else
	{
		ORIG_SlidingAndOtherStuff = nullptr;
		DevWarning("[server.dll] Could not find the sliding code!\n");
		Warning("y_spt_on_slide_pause_for has no effect.\n");
	}
}

void DMoMM::LoadFeature()
{
	sliding = false;
	wasSliding = false;
}

void DMoMM::UnloadFeature() {}

void __fastcall DMoMM::HOOKED_SlidingAndOtherStuff(void* thisptr, int edx, void* a, void* b)
{
	if (_dmomm.sliding)
	{
		_dmomm.sliding = false;
		_dmomm.wasSliding = true;
	}
	else
	{
		_dmomm.wasSliding = false;
	}

	return _dmomm.ORIG_SlidingAndOtherStuff(thisptr, edx, a, b);
}

void DMoMM::HOOKED_MiddleOfSlidingFunction_Func()
{
	sliding = true;

	if (!wasSliding)
	{
		const auto pauseFor = y_spt_on_slide_pause_for.GetInt();
		if (pauseFor > 0)
		{
			EngineConCmd("setpause");

			afterframes_entry_t entry;
			entry.framesLeft = pauseFor;
			entry.command = "unpause";
			_afterframes.AddAfterFramesEntry(entry);
		}
	}
}

__declspec(naked) void DMoMM::HOOKED_MiddleOfSlidingFunction()
{
	/**
		 * This is a hook in the middle of a function.
		 * Save all registers and EFLAGS to restore right before jumping out.
		 * Don't use local variables as they will corrupt the stack.
		 */
	__asm {
			pushad;
			pushfd;
	}

	_dmomm.HOOKED_MiddleOfSlidingFunction_Func();

	__asm {
			popfd;
			popad;
		/**
			 * It's important that nothing modifies the registers, the EFLAGS or the stack past this point,
			 * or the game won't be able to continue normally.
			 */

			jmp _dmomm.ORIG_MiddleOfSlidingFunction;
	}
}
