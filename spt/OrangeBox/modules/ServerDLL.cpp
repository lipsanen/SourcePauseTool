#include "stdafx.h"
#include "..\cvars.hpp"
#include "..\modules.hpp"

#include <SPTLib\hooks.hpp>
#include <SPTLib\memutils.hpp>
#include "..\..\sptlib-wrapper.hpp"
#include "..\..\utils\ent_utils.hpp"
#include "..\overlay\overlays.hpp"
#include "..\patterns.hpp"
#include "ServerDLL.hpp"
#include "..\..\utils\game_detection.hpp"
#include "..\..\features\afterframes.hpp"
#include "..\..\features\autojump.hpp"
#include "..\spt-serverplugin.hpp"

#ifdef OE
#include "SDK\usercmd.h"
#else
#include "..\game\shared\usercmd.h"
#endif

using std::size_t;
using std::uintptr_t;

void __fastcall ServerDLL::HOOKED_FinishGravity(void* thisptr, int edx)
{
	TRACE_ENTER();
	return serverDLL.HOOKED_FinishGravity_Func(thisptr, edx);
}

#define PRINT_FIND(future_name) \
	{ \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[server dll] Found " #future_name " at %p (using the %s pattern).\n", \
			       (unsigned int)ORIG_##future_name - (unsigned int)moduleBase, \
			       pattern->name()); \
		} \
		else \
		{ \
			DevWarning("[server dll] Could not find " #future_name ".\n"); \
		} \
	}

#define PRINT_FIND_VFTABLE(future_name) \
	{ \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[server dll] Found " #future_name " at %p (using the vftable).\n", \
			       (unsigned int)ORIG_##future_name - (unsigned int)moduleBase); \
		} \
		else \
		{ \
			DevWarning("[server dll] Could not find " #future_name ".\n"); \
		} \
	}

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::server::##name);
#define GET_HOOKEDFUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		PRINT_FIND(future_name) \
		if (ORIG_##future_name) \
		{ \
			patternContainer.AddHook(HOOKED_##future_name, (PVOID*)&ORIG_##future_name); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::server::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
	}

#define GET_FUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		PRINT_FIND(future_name) \
		if (ORIG_##future_name) \
		{ \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::server::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
	}

void ServerDLL::Hook(const std::wstring& moduleName,
                     void* moduleHandle,
                     void* moduleBase,
                     size_t moduleLength,
                     bool needToIntercept)
{
	Clear(); // Just in case.
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	patternContainer.Init(moduleName);

	DEF_FUTURE(FinishGravity);
	GET_HOOKEDFUTURE(FinishGravity);

	// FinishGravity
	if (ORIG_FinishGravity)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_FinishGravity);
		switch (ptnNumber)
		{
		case 0:
			off1M_bDucked = 1;
			off2M_bDucked = 2128;
			break;

		case 1:
			off1M_bDucked = 2;
			off2M_bDucked = 3120;
			break;

		case 2:
			off1M_bDucked = 2;
			off2M_bDucked = 3184;
			break;

		case 3:
			off1M_bDucked = 2;
			off2M_bDucked = 3376;
			break;

		case 4:
			off1M_bDucked = 1;
			off2M_bDucked = 3440;
			break;

		case 5:
			off1M_bDucked = 1;
			off2M_bDucked = 3500;
			break;

		case 6:
			off1M_bDucked = 1;
			off2M_bDucked = 3724;
			break;

		case 7:
			off1M_bDucked = 2;
			off2M_bDucked = 3112;
			break;

		case 8:
			off1M_bDucked = 1;
			off2M_bDucked = 3416;
			break;
		}
	}
	else
	{
		Warning("y_spt_additional_jumpboost has no effect.\n");
	}

	patternContainer.Hook();
}

void ServerDLL::Unhook()
{
	patternContainer.Unhook();
	Clear();
}

void ServerDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_CheckJumpButton = nullptr;
	ORIG_FinishGravity = nullptr;
	ORIG_PlayerRunCommand = nullptr;
	off1M_bDucked = 0;
	off2M_bDucked = 0;
}

void __fastcall ServerDLL::HOOKED_FinishGravity_Func(void* thisptr, int edx)
{
	if (_autojump.insideCheckJumpButton && y_spt_additional_jumpboost.GetBool())
	{
		CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t*)thisptr + _autojump.off1M_nOldButtons));
		bool ducked = *(bool*)(*((uintptr_t*)thisptr + off1M_bDucked) + off2M_bDucked);

		// <stolen from gamemovement.cpp>
		{
			Vector vecForward;
			AngleVectors(mv->m_vecViewAngles, &vecForward);
			vecForward.z = 0;
			VectorNormalize(vecForward);

			// We give a certain percentage of the current forward movement as a bonus to the jump speed.  That bonus is clipped
			// to not accumulate over time.
			float flSpeedBoostPerc = (!mv->m_bIsSprinting && !ducked) ? 0.5f : 0.1f;
			float flSpeedAddition = fabs(mv->m_flForwardMove * flSpeedBoostPerc);
			float flMaxSpeed = mv->m_flMaxSpeed + (mv->m_flMaxSpeed * flSpeedBoostPerc);
			float flNewSpeed = (flSpeedAddition + mv->m_vecVelocity.Length2D());

			// If we're over the maximum, we want to only boost as much as will get us to the goal speed
			if (y_spt_additional_jumpboost.GetInt() == 1)
			{
				if (flNewSpeed > flMaxSpeed)
				{
					flSpeedAddition -= flNewSpeed - flMaxSpeed;
				}

				if (mv->m_flForwardMove < 0.0f)
					flSpeedAddition *= -1.0f;
			}

			// Add it on
			VectorAdd((vecForward * flSpeedAddition), mv->m_vecVelocity, mv->m_vecVelocity);
		}
		// </stolen from gamemovement.cpp>
	}

	return ORIG_FinishGravity(thisptr, edx);
}
