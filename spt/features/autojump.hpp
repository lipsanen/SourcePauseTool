#pragma once

#include "..\feature.hpp"
#include "thirdparty\Signal.h"

#if defined(OE)
#include "vector.h"
#else
#include "mathlib\vector.h"
#endif

typedef bool(__fastcall* _CheckJumpButton_client)(void* thisptr, int edx);

// y_spt_autojump
class AutojumpFeature : public Feature
{
public:
	bool cantJumpNextTime = false;
	bool insideCheckJumpButton = false;

protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	virtual void PreHook() override;

private:
	static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	static bool __fastcall HOOKED_CheckJumpButton_client(void* thisptr, int edx);
	static void __fastcall HOOKED_FinishGravity(void* thisptr, int edx);

	_CheckJumpButton_client ORIG_CheckJumpButton_client = nullptr;

	bool client_cantJumpNextTime = false;
	bool client_insideCheckJumpButton = false;
};

extern AutojumpFeature spt_autojump;