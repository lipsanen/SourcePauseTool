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

int __fastcall ServerDLL::HOOKED_CheckStuck(void* thisptr, int edx)
{
	TRACE_ENTER();
	return serverDLL.HOOKED_CheckStuck_Func(thisptr, edx);
}

void __fastcall ServerDLL::HOOKED_AirAccelerate(void* thisptr, int edx, Vector* wishdir, float wishspeed, float accel)
{
	TRACE_ENTER();
	return serverDLL.HOOKED_AirAccelerate_Func(thisptr, edx, wishdir, wishspeed, accel);
}

void __fastcall ServerDLL::HOOKED_ProcessMovement(void* thisptr, int edx, void* pPlayer, void* pMove)
{
	TRACE_ENTER();
	return serverDLL.HOOKED_ProcessMovement_Func(thisptr, edx, pPlayer, pMove);
}

__declspec(naked) void ServerDLL::HOOKED_MiddleOfTeleportTouchingEntity()
{
	/**
	* We want a pointer to the portal and the coords of whatever is being teleported. 
	* The former is currently in ebp (which is not used here as the stack frame pointer), and the latter
	* is somewhere on the stack.
	*/
	__asm {
		pushad;
		pushfd;
		mov ecx, ebp; // first paremeter - pass portal ref
		mov edx, esp; // second parameter - pass stack pointer
		add edx, 0x24; // account for pushad/pushfd
		call ServerDLL::HOOKED_MiddleOfTeleportTouchingEntity_Func;
		popfd;
		popad;
		jmp serverDLL.ORIG_MiddleOfTeleportTouchingEntity;
	}
}

__declspec(naked) void ServerDLL::HOOKED_EndOfTeleportTouchingEntity()
{
	__asm {
		pushad;
		pushfd;
	}
	serverDLL.HOOKED_EndOfTeleportTouchingEntity_Func();
	__asm {
		popfd;
		popad;
		jmp serverDLL.ORIG_EndOfTeleportTouchingEntity;
	}
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
	DEF_FUTURE(CheckStuck);
	DEF_FUTURE(CheckJumpButton);
	DEF_FUTURE(SetPredictionRandomSeed);
	GET_HOOKEDFUTURE(FinishGravity);
	GET_HOOKEDFUTURE(CheckStuck);
	GET_HOOKEDFUTURE(SetPredictionRandomSeed);

	if (utils::DoesGameLookLikePortal())
	{
		DEF_FUTURE(MiddleOfTeleportTouchingEntity);
		DEF_FUTURE(EndOfTeleportTouchingEntity);
		GET_HOOKEDFUTURE(MiddleOfTeleportTouchingEntity);
		GET_HOOKEDFUTURE(EndOfTeleportTouchingEntity);
	}

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

	// CheckStuck
	if (!ORIG_CheckStuck)
	{
		Warning("y_spt_stucksave has no effect.\n");
	}

	if (!ORIG_MiddleOfTeleportTouchingEntity || !ORIG_EndOfTeleportTouchingEntity)
	{
		DevWarning("[server.dll] Could not find the teleport function!\n");
	}

	extern void* gm;
	if (gm)
	{
		auto vftable = *reinterpret_cast<void***>(gm);
		//ORIG_AirAccelerate = reinterpret_cast<_AirAccelerate>(MemUtils::HookVTable(vftable, 17, reinterpret_cast<uintptr_t>(HOOKED_AirAccelerate)));
		patternContainer.AddVFTableHook(
		    VFTableHook(vftable, 1, (PVOID)HOOKED_ProcessMovement, (PVOID*)&ORIG_ProcessMovement));
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
	ORIG_CheckStuck = nullptr;
	ORIG_AirAccelerate = nullptr;
	ORIG_MiddleOfTeleportTouchingEntity = nullptr;
	ORIG_EndOfTeleportTouchingEntity = nullptr;
	off1M_bDucked = 0;
	off2M_bDucked = 0;
	ticksPassed = 0;
	timerRunning = false;
	commandNumber = 0;
	recursiveTeleportCount = 0;
}

int ServerDLL::GetCommandNumber()
{
	return commandNumber;
}

void __cdecl ServerDLL::HOOKED_SetPredictionRandomSeed(void* usercmd)
{
	CUserCmd* ptr = reinterpret_cast<CUserCmd*>(usercmd);
	if (ptr)
	{
		serverDLL.commandNumber = ptr->command_number;
	}

	serverDLL.ORIG_SetPredictionRandomSeed(usercmd);
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

int __fastcall ServerDLL::HOOKED_CheckStuck_Func(void* thisptr, int edx)
{
	auto ret = ORIG_CheckStuck(thisptr, edx);

	if (ret && y_spt_stucksave.GetString()[0] != '\0')
	{
		std::ostringstream oss;
		oss << "save " << y_spt_stucksave.GetString();
		EngineConCmd(oss.str().c_str());
		y_spt_stucksave.SetValue("");
	}

	return ret;
}

void __fastcall ServerDLL::HOOKED_AirAccelerate_Func(void* thisptr,
                                                     int edx,
                                                     Vector* wishdir,
                                                     float wishspeed,
                                                     float accel)
{
	const double M_RAD2DEG = 180 / M_PI;

	CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t*)thisptr + _autojump.off1M_nOldButtons));
	DevMsg("[AA Pre ] velocity: %.8f %.8f %.8f\n", mv->m_vecVelocity.x, mv->m_vecVelocity.y, mv->m_vecVelocity.z);
	DevMsg("[AA Pre ] speed = %.8f; wishspeed = %.8f; accel = %.8f; wishdir = %.8f; surface friction = %.8f\n",
	       mv->m_vecVelocity.Length2D(),
	       wishspeed,
	       accel,
	       atan2(wishdir->y, wishdir->x) * M_RAD2DEG,
	       *(float*)(*(uintptr_t*)((uintptr_t)thisptr + 4) + 3812));

	ORIG_AirAccelerate(thisptr, edx, wishdir, wishspeed, accel);

	DevMsg("[AA Post] speed = %.8f\n", mv->m_vecVelocity.Length2D());
}

void __fastcall ServerDLL::HOOKED_ProcessMovement_Func(void* thisptr, int edx, void* pPlayer, void* pMove)
{
	CHLMoveData* mv = reinterpret_cast<CHLMoveData*>(pMove);
	if (tas_log.GetBool())
		DevMsg("[ProcessMovement PRE ] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
		       mv->GetAbsOrigin().x,
		       mv->GetAbsOrigin().y,
		       mv->GetAbsOrigin().z,
		       mv->m_vecVelocity.x,
		       mv->m_vecVelocity.y,
		       mv->m_vecVelocity.z);

	ORIG_ProcessMovement(thisptr, edx, pPlayer, pMove);

	if (tas_log.GetBool())
		DevMsg("[ProcessMovement POST] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
		       mv->GetAbsOrigin().x,
		       mv->GetAbsOrigin().y,
		       mv->GetAbsOrigin().z,
		       mv->m_vecVelocity.x,
		       mv->m_vecVelocity.y,
		       mv->m_vecVelocity.z);
}

/**
* A no free edicts crash when trying to do a vag happens when the 2nd teleport places the entity
* behind the entry portal. This causes another teleport by the entry portal to be queued which
* sometimes places the entity right back to where it started, triggering another vag. This process is
* recursive, and would eventually cause a stack overflow if the game didn't crash from allocating an
* edict for a shadowclone every single teleport. This function detects when there are too many recursive
* teleports, and nudges the entity position before the teleport so that it doesn't return to exactly the
* same spot. The position vector is on the stack at this point, so we access it via a stack pointer from
* the original teleport function. This works for both players and other entities.
*/
void __fastcall ServerDLL::HOOKED_MiddleOfTeleportTouchingEntity_Func(void* portalPtr, void* tpStackPointer)
{
	if (!serverDLL.ORIG_EndOfTeleportTouchingEntity || !y_spt_prevent_vag_crash.GetBool())
		return;
	if (serverDLL.recursiveTeleportCount++ > 2)
	{
		Msg("spt: nudging entity to prevent more recursive teleports!\n");
		Vector* entPos = (Vector*)((uint32_t*)tpStackPointer + 26);
		Vector* portalNorm = *((Vector**)portalPtr + 2505) + 2;
		DevMsg(
		    "spt: ent coords in TeleportTouchingEntity: %f %f %f, portal norm: %f %f %f, %i recursive teleports\n",
		    entPos->x,
		    entPos->y,
		    entPos->z,
		    portalNorm->x,
		    portalNorm->y,
		    portalNorm->z,
		    serverDLL.recursiveTeleportCount);
		// push entity further into the portal so it comes further out after the teleport
		entPos->x -= portalNorm->x;
		entPos->y -= portalNorm->y;
		entPos->z -= portalNorm->z;
	}
}

void ServerDLL::HOOKED_EndOfTeleportTouchingEntity_Func()
{
	if (y_spt_prevent_vag_crash.GetBool())
		recursiveTeleportCount--;
}

