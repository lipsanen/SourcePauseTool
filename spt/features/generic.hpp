#pragma once
#include "..\feature.hpp"
#include "thirdparty\Signal.h"

typedef void(__stdcall* _HudUpdate)(bool bActive);
typedef bool(__cdecl* _SV_ActivateServer)();
typedef void(__fastcall* _FinishRestore)(void* thisptr, int edx);
typedef void(__fastcall* _SetPaused)(void* thisptr, int edx, bool paused);

// For hooks used by many features
class GenericFeature : public Feature
{
public:
	Gallant::Signal0<void> TickSignal;
	Gallant::Signal1<bool> OngroundSignal;
	Gallant::Signal0<void> FrameSignal;
	Gallant::Signal1<bool> SV_ActivateServerSignal;
	Gallant::Signal2<void*, int> FinishRestoreSignal;
	Gallant::Signal3<void*, int, bool> SetPausedSignal;

	_HudUpdate ORIG_HudUpdate;
	_SetPaused ORIG_SetPaused;
	_SV_ActivateServer ORIG_SV_ActivateServer;
	_FinishRestore ORIG_FinishRestore;
	bool shouldPreventNextUnpause;

	void Tick();

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;
private:

	static void __stdcall HOOKED_HudUpdate(bool bActive);
	static bool __cdecl HOOKED_SV_ActivateServer();
	static void __fastcall HOOKED_FinishRestore(void* thisptr, int edx);
	static void __fastcall HOOKED_SetPaused(void* thisptr, int edx, bool paused);
};

extern GenericFeature generic_;
