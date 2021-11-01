#pragma once

#include "..\strafestuff.hpp"

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