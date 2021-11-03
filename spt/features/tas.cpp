#include "stdafx.h"
#include "..\feature.hpp"
#include "..\OrangeBox\spt-serverplugin.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\aim.hpp"
#include "..\strafestuff.hpp"
#include "..\OrangeBox\cvars.hpp"
#include "generic.hpp"
#include "playerio.hpp"

typedef void(__fastcall* _AdjustAngles)(void* thisptr, int edx, float frametime);

// Enables TAS strafing and view related functionality
class TASFeature : public Feature
{
private:
	void __fastcall HOOKED_AdjustAngles_Func(void* thisptr, int edx, float frametime);
	static void __fastcall HOOKED_AdjustAngles(void* thisptr, int edx, float frametime);
	void Strafe(float* va, bool yawChanged);

private:
	_AdjustAngles ORIG_AdjustAngles;
	bool tasAddressesWereFound;

protected:
	virtual bool ShouldLoadFeature() override
	{
		return GetEngine() != nullptr;
	}

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

static TASFeature _tas;

void TASFeature::InitHooks()
{
	HOOK_FUNCTION(client, AdjustAngles);
}

void TASFeature::LoadFeature() 
{
	tasAddressesWereFound = ORIG_AdjustAngles && playerio::PlayerIOAddressesWereFound();

	if(!tasAddressesWereFound)
		Warning("The full game TAS solutions are not available.\n");
}

void TASFeature::UnloadFeature() {}

void __fastcall TASFeature::HOOKED_AdjustAngles_Func(void* thisptr, int edx, float frametime)
{
	ORIG_AdjustAngles(thisptr, edx, frametime);
	playerio::Set_cinput_thisptr(thisptr);

	if(_playerio.pCmd == NULL)
		return;

	float va[3];
	bool yawChanged = false;
	EngineGetViewAngles(va);
	playerio::HandleAiming(va, yawChanged);

	if (tasAddressesWereFound && tas_strafe.GetBool())
	{
		Strafe(va, yawChanged);
	}

	EngineSetViewAngles(va);
}

void TASFeature::Strafe(float* va, bool yawChanged)
{
	auto vars = playerio::GetMovementVars();
	auto pl = playerio::GetPlayerData();

	bool jumped = false;

	auto btns = Strafe::StrafeButtons();
	bool usingButtons = (sscanf(tas_strafe_buttons.GetString(),
	                            "%hhu %hhu %hhu %hhu",
	                            &btns.AirLeft,
	                            &btns.AirRight,
	                            &btns.GroundLeft,
	                            &btns.GroundRight)
	                     == 4);
	auto type = static_cast<Strafe::StrafeType>(tas_strafe_type.GetInt());
	auto dir = static_cast<Strafe::StrafeDir>(tas_strafe_dir.GetInt());

	Strafe::ProcessedFrame out;
	out.Jump = false;

	if (!vars.CantJump && vars.OnGround)
	{
		if (tas_strafe_lgagst.GetBool())
		{
			bool jump = Strafe::LgagstJump(pl, vars);
			if (jump)
			{
				vars.OnGround = false;
				out.Jump = true;
				jumped = true;
			}
		}

		if (playerio::TryJump())
		{
			vars.OnGround = false;
			jumped = true;
		}
	}

	Strafe::Friction(pl, vars.OnGround, vars);

	if (tas_strafe_vectorial
	        .GetBool()) // Can do vectorial strafing even with locked camera, provided we are not jumping
		Strafe::StrafeVectorial(pl,
		                        vars,
		                        jumped,
		                        type,
		                        dir,
		                        tas_strafe_yaw.GetFloat(),
		                        va[YAW],
		                        out,
		                        yawChanged);
	else if (!yawChanged) // not changing yaw, can do regular strafe
		Strafe::Strafe(
		    pl, vars, jumped, type, dir, tas_strafe_yaw.GetFloat(), va[YAW], out, btns, usingButtons);


	playerio::SetTASInput(va, out);
}

void __fastcall TASFeature::HOOKED_AdjustAngles(void* thisptr, int edx, float frametime)
{
	_tas.HOOKED_AdjustAngles_Func(thisptr, edx, frametime);
}
