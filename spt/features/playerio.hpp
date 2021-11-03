#pragma once

#include "..\feature.hpp"
#include "..\strafestuff.hpp"


typedef void(__fastcall* _CalcAbsoluteVelocity)(void* thisptr, int edx);
typedef void* (__fastcall* _GetGroundEntity)(void* thisptr, int edx);
typedef void(
	__fastcall* _CreateMove)(void* thisptr, int edx, int sequence_number, float input_sample_frametime, bool active);
typedef int(__fastcall* _GetButtonBits)(void* thisptr, int edx, int bResetState);
typedef void* (__cdecl* _GetLocalPlayer)();

typedef struct
{
	float angle;
	bool set;
} angset_command_t;

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

	static void GetGroundEntity_callback(patterns::PatternWrapper* found, int index);
	static void MiddleOfCAM_Think_callback(patterns::PatternWrapper* found, int index);
	static void CreateMove_callback(patterns::PatternWrapper* found, int index);

public:
	void SetTASInput(float* va, const Strafe::ProcessedFrame& out);
	void HandleAiming(float* va, bool& yawChanged);
	bool DoAngleChange(float& angle, float target);
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

	bool duckspam;
	bool forceJump;
	bool forceUnduck;
	angset_command_t setPitch, setYaw;
	bool playerioAddressesWereFound;
	ptrdiff_t offServerAbsOrigin;
	uintptr_t pCmd;

	void EnableDuckspam()
	{
		duckspam = true;
	}
	void DisableDuckspam()
	{
		duckspam = false;
	}

	void SetPitch(float pitch)
	{
		setPitch.angle = pitch;
		setPitch.set = true;
	}
	void SetYaw(float yaw)
	{
		setYaw.angle = yaw;
		setYaw.set = true;
	}
	void ResetPitchYawCommands()
	{
		setYaw.set = false;
		setPitch.set = false;
	}

private:
	_GetGroundEntity ORIG_GetGroundEntity;
	_CreateMove ORIG_CreateMove;
	_GetButtonBits ORIG_GetButtonBits;
	_GetLocalPlayer ORIG_GetLocalPlayer;
	_CalcAbsoluteVelocity ORIG_CalcAbsoluteVelocity;
	uintptr_t ORIG_MiddleOfCAM_Think;

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
};

extern PlayerIOFeature _playerio;


namespace playerio
{
	void SetTASInput(float* va, const Strafe::ProcessedFrame& out);
	void HandleAiming(float* va, bool& yawChanged);
	bool TryJump();
	void Set_cinput_thisptr(void* thisptr);
	Strafe::PlayerData GetPlayerData();
	Strafe::MovementVars GetMovementVars();
	bool PlayerIOAddressesWereFound();
	Vector GetPlayerVelocity();
	Vector GetPlayerEyePos();
	int GetPlayerFlags();
	bool GetFlagsDucking();
	bool IsGroundEntitySet();
} // namespace playerio