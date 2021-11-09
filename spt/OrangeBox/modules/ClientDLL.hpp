#include "..\..\stdafx.hpp"
#pragma once

#include <vector>

#include <SPTLib\IHookableNameFilter.hpp>
#include "..\spt-serverplugin.hpp"
#include "..\..\..\SDK\igamemovement.h"
#include "..\..\strafestuff.hpp"
#include "..\..\utils\patterncontainer.hpp"
#include "..\public\cdll_int.h"
#include "thirdparty\Signal.h"
#include "cmodel.h"

using std::size_t;
using std::uintptr_t;

typedef void(__fastcall* _CViewRender__OnRenderStart)(void* thisptr, int edx);


class ClientDLL : public IHookableNameFilter
{
public:
	ClientDLL() : IHookableNameFilter({L"client.dll"}){};
	virtual void Hook(const std::wstring& moduleName,
	                  void* moduleHandle,
	                  void* moduleBase,
	                  size_t moduleLength,
	                  bool needToIntercept);
	virtual void Unhook();
	virtual void Clear();

	static void __fastcall HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx);
	void __fastcall HOOKED_CViewRender__OnRenderStart_Func(void* thisptr, int edx);

protected:
	_CViewRender__OnRenderStart ORIG_CViewRender__OnRenderStart;

protected:
	PatternContainer patternContainer;
};
