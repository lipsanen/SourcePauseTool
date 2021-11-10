#include "stdafx.h"
#include "autojump.hpp"

#include "basehandle.h"
#include "SDK\hl_movedata.h"
#include "convar.h"
#include "dbg.h"
#include "..\OrangeBox\cvars.hpp"

AutojumpFeature _autojump;
ConVar y_spt_autojump("y_spt_autojump", "0", FCVAR_ARCHIVE);
ConVar _y_spt_autojump_ensure_legit("_y_spt_autojump_ensure_legit", "1", FCVAR_CHEAT);
ConVar y_spt_additional_jumpboost("y_spt_additional_jumpboost",
                                  "0",
                                  0,
                                  "1 = Add in ABH movement mechanic, 2 = Add in OE movement mechanic.\n");

bool AutojumpFeature::ShouldLoadFeature()
{
	return true;
}

void AutojumpFeature::InitHooks()
{
	HOOK_FUNCTION(server, CheckJumpButton);
	HOOK_FUNCTION(client, CheckJumpButton_client);
	HOOK_FUNCTION(server, FinishGravity);
}

void AutojumpFeature::LoadFeature()
{
	// Server-side CheckJumpButton
	if (ORIG_CheckJumpButton)
	{
		int ptnNumber = GetPatternIndex((PVOID*)&ORIG_CheckJumpButton);
		switch (ptnNumber)
		{
		case 0:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 1:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 2:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 3:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 4:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 5:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 6:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 7:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 8:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 9:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 10:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 11:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 12:
			off1M_nOldButtons = 3;
			off2M_nOldButtons = 40;
			break;

		case 13:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 14:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 15:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 16:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 17:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 18:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;
		}
	}
	else
	{
		Warning("y_spt_autojump has no effect.\n");
	}

	if (ORIG_FinishGravity)
	{
		int ptnNumber = GetPatternIndex((PVOID*)&ORIG_FinishGravity);
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
		off1M_bDucked = 0;
		off2M_bDucked = 0;
	}
}

void AutojumpFeature::UnloadFeature() {}

bool __fastcall AutojumpFeature::HOOKED_CheckJumpButton(void* thisptr, int edx)
{
	const int IN_JUMP = (1 << 1);

	int* pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t*)thisptr + _autojump.off1M_nOldButtons));
	if (tas_log.GetBool())
		DevMsg("[CheckJumpButton PRE ] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
		       mv->GetAbsOrigin().x,
		       mv->GetAbsOrigin().y,
		       mv->GetAbsOrigin().z,
		       mv->m_vecVelocity.x,
		       mv->m_vecVelocity.y,
		       mv->m_vecVelocity.z);

	if (y_spt_autojump.GetBool())
	{
		pM_nOldButtons =
		    (int*)(*((uintptr_t*)thisptr + _autojump.off1M_nOldButtons) + _autojump.off2M_nOldButtons);
		origM_nOldButtons = *pM_nOldButtons;

		if (!_autojump.cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
	}

	_autojump.cantJumpNextTime = false;

	_autojump.insideCheckJumpButton = true;
	bool rv = _autojump.ORIG_CheckJumpButton(thisptr, edx); // This function can only change the jump bit.
	_autojump.insideCheckJumpButton = false;

	if (y_spt_autojump.GetBool())
	{
		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
		{
			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
		}
	}

	if (rv)
	{
		// We jumped.
		if (_y_spt_autojump_ensure_legit.GetBool())
		{
			_autojump.JumpSignal();
			_autojump.cantJumpNextTime = true; // Prevent consecutive jumps.
		}
	}

	if (tas_log.GetBool())
		DevMsg("[CheckJumpButton POST] origin: %.8f %.8f %.8f; velocity: %.8f %.8f %.8f\n",
		       mv->GetAbsOrigin().x,
		       mv->GetAbsOrigin().y,
		       mv->GetAbsOrigin().z,
		       mv->m_vecVelocity.x,
		       mv->m_vecVelocity.y,
		       mv->m_vecVelocity.z);

	return rv;
}

bool __fastcall AutojumpFeature::HOOKED_CheckJumpButton_client(void* thisptr, int edx)
{
	/*
	Not sure if this gets called at all from the client dll, but
	I will just hook it in exactly the same way as the server one.
	*/
	const int IN_JUMP = (1 << 1);

	int* pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	if (y_spt_autojump.GetBool())
	{
		pM_nOldButtons =
		    (int*)(*((uintptr_t*)thisptr + _autojump.off1M_nOldButtons) + _autojump.off2M_nOldButtons);
		origM_nOldButtons = *pM_nOldButtons;

		if (!_autojump.client_cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
	}

	_autojump.client_cantJumpNextTime = false;

	_autojump.client_insideCheckJumpButton = true;
	bool rv = _autojump.ORIG_CheckJumpButton_client(thisptr, edx); // This function can only change the jump bit.
	_autojump.client_insideCheckJumpButton = false;

	if (y_spt_autojump.GetBool())
	{
		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
		{
			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
		}
	}

	if (rv)
	{
		// We jumped.
		if (_y_spt_autojump_ensure_legit.GetBool())
		{
			_autojump.client_cantJumpNextTime = true; // Prevent consecutive jumps.
		}
	}

	DevMsg("Engine call: [client dll] CheckJumpButton() => %s\n", (rv ? "true" : "false"));

	return rv;
}

void __fastcall AutojumpFeature::HOOKED_FinishGravity(void* thisptr, int edx)
{
	if (_autojump.insideCheckJumpButton && y_spt_additional_jumpboost.GetBool())
	{
		CHLMoveData* mv = (CHLMoveData*)(*((uintptr_t*)thisptr + _autojump.off1M_nOldButtons));
		bool ducked = *(bool*)(*((uintptr_t*)thisptr + _autojump.off1M_bDucked) + _autojump.off2M_bDucked);

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

	return _autojump.ORIG_FinishGravity(thisptr, edx);
}
