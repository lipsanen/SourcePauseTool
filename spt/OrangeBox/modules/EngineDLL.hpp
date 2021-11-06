#include "..\..\stdafx.hpp"
#pragma once

#include <SPTLib\IHookableNameFilter.hpp>

#include "..\..\utils\patterncontainer.hpp"

#ifdef OE
#include "mathlib.h"
#else
#include "mathlib/mathlib.h"
#endif

using std::size_t;
using std::uintptr_t;

typedef bool(__fastcall* _CEngineTrace__PointOutsideWorld)(void* thisptr, int edx, const Vector& pt);

class EngineDLL : public IHookableNameFilter
{
public:
	EngineDLL() : IHookableNameFilter({L"engine.dll"}){};
	virtual void Hook(const std::wstring& moduleName,
	                  void* moduleHandle,
	                  void* moduleBase,
	                  size_t moduleLength,
	                  bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();


	_CEngineTrace__PointOutsideWorld ORIG_CEngineTrace__PointOutsideWorld;

protected:
	PatternContainer patternContainer;

	void* pGameServer;
	bool* pM_bLoadgame;
	bool shouldPreventNextUnpause;
	int* pM_State;
	int* pM_nSignonState;
};
