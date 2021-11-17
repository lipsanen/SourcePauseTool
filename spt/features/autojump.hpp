#pragma once

#include "..\feature.hpp"
#include "thirdparty\Signal.h"

#if defined(OE)
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

typedef bool(__fastcall* _CheckJumpButton)(void* thisptr, int edx);
typedef bool(__fastcall* _CheckJumpButton_client)(void* thisptr, int edx);
typedef void(__fastcall* _FinishGravity)(void* thisptr, int edx);

// y_spt_autojump
class AutojumpFeature : public Feature
{
public:
	Gallant::Signal0<void> JumpSignal;
	ptrdiff_t off1M_nOldButtons;
	ptrdiff_t off2M_nOldButtons;
	bool cantJumpNextTime;
	bool insideCheckJumpButton;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	static bool __fastcall HOOKED_CheckJumpButton_client(void* thisptr, int edx);
	static void __fastcall HOOKED_FinishGravity(void* thisptr, int edx);

	_CheckJumpButton ORIG_CheckJumpButton;
	_CheckJumpButton_client ORIG_CheckJumpButton_client;
	_FinishGravity ORIG_FinishGravity;
	Vector oldVel;
	ptrdiff_t off1M_bDucked;
	ptrdiff_t off2M_bDucked;

	bool client_cantJumpNextTime;
	bool client_insideCheckJumpButton;
};

extern AutojumpFeature _autojump;