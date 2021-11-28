#pragma once

#include "..\feature.hpp"
#include "..\strafe\strafestuff.hpp"

typedef void(__fastcall* _CalcAbsoluteVelocity)(void* thisptr, int edx);
typedef void*(__fastcall* _GetGroundEntity)(void* thisptr, int edx);
typedef void(
    __fastcall* _CreateMove)(void* thisptr, int edx, int sequence_number, float input_sample_frametime, bool active);
typedef int(__fastcall* _GetButtonBits)(void* thisptr, int edx, int bResetState);
typedef void*(__cdecl* _GetLocalPlayer)();

// This feature reads player stuff from memory and writes player stuff into memory
class PlayerIOFeature : public Feature
{
private:
	void __fastcall HOOKED_CreateMove_Func(void* thisptr,
	                                       int edx,
	                                       int sequence_number,
	                                       float input_sample_frametime,
	                                       bool active);
	static void __fastcall HOOKED_CreateMove(void* thisptr,
	                                         int edx,
	                                         int sequence_number,
	                                         float input_sample_frametime,
	                                         bool active);
	int __fastcall HOOKED_GetButtonBits_Func(void* thisptr, int edx, int bResetState);
	static int __fastcall HOOKED_GetButtonBits(void* thisptr, int edx, int bResetState);

public:
	void SetTASInput(float* va, const Strafe::ProcessedFrame& out);
	Strafe::MovementVars GetMovementVars();
	bool GetFlagsDucking();
	Strafe::PlayerData GetPlayerData();
	Vector GetPlayerVelocity();
	Vector GetPlayerEyePos();
	int GetPlayerFlags();
	double GetDuckJumpTime();
	bool IsGroundEntitySet();
	bool TryJump();
	bool PlayerIOAddressesFound();
	int GetPlayerPhysicsFlags() const;
	int GetPlayerMoveType() const;
	int GetPlayerMoveCollide() const;
	int GetPlayerCollisionGroup() const;
	void Set_cinput_thisptr(void* thisptr);

	bool duckspam;
	bool forceJump;
	bool forceUnduck;
	bool playerioAddressesWereFound;
	ptrdiff_t offServerAbsOrigin;
	uintptr_t pCmd;

	ptrdiff_t offM_vecAbsVelocity;
	ptrdiff_t offM_afPhysicsFlags;
	ptrdiff_t offM_moveType;
	ptrdiff_t offM_moveCollide;
	ptrdiff_t offM_collisionGroup;
	ptrdiff_t offM_vecPunchAngle;
	ptrdiff_t offM_vecPunchAngleVel;

	void EnableDuckspam()
	{
		duckspam = true;
	}
	void DisableDuckspam()
	{
		duckspam = false;
	}

private:
	_GetGroundEntity ORIG_GetGroundEntity;
	_CreateMove ORIG_CreateMove;
	_GetButtonBits ORIG_GetButtonBits;
	_GetLocalPlayer ORIG_GetLocalPlayer;
	_CalcAbsoluteVelocity ORIG_CalcAbsoluteVelocity;
	uintptr_t ORIG_MiddleOfCAM_Think;
	uintptr_t ORIG_PlayerRunCommand;

	ptrdiff_t offM_pCommands;
	ptrdiff_t offForwardmove;
	ptrdiff_t offSidemove;
	ptrdiff_t offMaxspeed;
	ptrdiff_t offFlags;
	ptrdiff_t offAbsVelocity;
	ptrdiff_t offDucking;
	ptrdiff_t offDuckJumpTime;
	ptrdiff_t offServerSurfaceFriction;
	ptrdiff_t offServerPreviouslyPredictedOrigin;
	std::size_t sizeofCUserCmd;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	virtual void PreHook() override;
};

extern PlayerIOFeature spt_playerio;
