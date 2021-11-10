#include "..\..\stdafx.hpp"
#pragma once

#include "cmodel.h"
#include <SPTLib\IHookableNameFilter.hpp>

#include <SDK\hl_movedata.h>
#include "..\..\utils\patterncontainer.hpp"
#include "engine\iserverplugin.h"
#include "trace.h"

using std::ptrdiff_t;
using std::size_t;
using std::uintptr_t;

typedef bool(__fastcall* _CheckJumpButton)(void* thisptr, int edx);
typedef void(__fastcall* _FinishGravity)(void* thisptr, int edx);
typedef void(__fastcall* _PlayerRunCommand)(void* thisptr, int edx, void* ucmd, void* moveHelper);
typedef void(__cdecl* _SetPredictionRandomSeed)(void* usercmd);

class ServerDLL : public IHookableNameFilter
{
public:
	ServerDLL() : IHookableNameFilter({L"server.dll"}){};
	virtual void Hook(const std::wstring& moduleName,
	                  void* moduleHandle,
	                  void* moduleBase,
	                  size_t moduleLength,
	                  bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();

	static void __fastcall HOOKED_FinishGravity(void* thisptr, int edx);
	static void __fastcall HOOKED_SlidingAndOtherStuff(void* thisptr, int edx, void* a, void* b);
	static int __fastcall HOOKED_CRestore__ReadAll(void* thisptr, int edx, void* pLeafObject, datamap_t* pLeafMap);
	static int __fastcall HOOKED_CRestore__DoReadAll(void* thisptr,
	                                                 int edx,
	                                                 void* pLeafObject,
	                                                 datamap_t* pLeafMap,
	                                                 datamap_t* pCurMap);
	static int __cdecl HOOKED_DispatchSpawn(void* pEntity);
	static void HOOKED_MiddleOfTeleportTouchingEntity();
	static void HOOKED_EndOfTeleportTouchingEntity();
	static void __cdecl HOOKED_SetPredictionRandomSeed(void* usercmd);
	void __fastcall HOOKED_FinishGravity_Func(void* thisptr, int edx);
	int __fastcall HOOKED_CheckStuck_Func(void* thisptr, int edx);
	void HOOKED_EndOfTeleportTouchingEntity_Func();
	static void __fastcall HOOKED_MiddleOfTeleportTouchingEntity_Func(void* portalPtr, void* tpStackPointer);
	int GetCommandNumber();

	void StartTimer()
	{
		timerRunning = true;
	}
	void StopTimer()
	{
		timerRunning = false;
	}
	void ResetTimer()
	{
		ticksPassed = 0;
		timerRunning = false;
	}
	unsigned int GetTicksPassed() const
	{
		return ticksPassed;
	}


protected:
	PatternContainer patternContainer;
	_CheckJumpButton ORIG_CheckJumpButton;
	_FinishGravity ORIG_FinishGravity;
	_PlayerRunCommand ORIG_PlayerRunCommand;
	void* ORIG_MiddleOfTeleportTouchingEntity;
	void* ORIG_EndOfTeleportTouchingEntity;
	_SetPredictionRandomSeed ORIG_SetPredictionRandomSeed;

	int commandNumber;
	ptrdiff_t off1M_bDucked;
	ptrdiff_t off2M_bDucked;

	unsigned ticksPassed;
	bool timerRunning;

	int recursiveTeleportCount;
};
