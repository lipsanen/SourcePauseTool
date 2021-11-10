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
	void __fastcall HOOKED_FinishGravity_Func(void* thisptr, int edx);
	int __fastcall HOOKED_CheckStuck_Func(void* thisptr, int edx);

protected:
	PatternContainer patternContainer;
	_CheckJumpButton ORIG_CheckJumpButton;
	_FinishGravity ORIG_FinishGravity;
	_PlayerRunCommand ORIG_PlayerRunCommand;

	ptrdiff_t off1M_bDucked;
	ptrdiff_t off2M_bDucked;
};
