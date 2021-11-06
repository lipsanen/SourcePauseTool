#include "stdafx.h"

#include "ClientDLL.hpp"

#include <SPTLib\hooks.hpp>
#include <SPTLib\memutils.hpp>

#include "..\..\sptlib-wrapper.hpp"
#include "..\..\strafestuff.hpp"
#include "..\cvars.hpp"
#include "..\modules.hpp"
#include "..\overlay\overlay-renderer.hpp"
#include "..\patterns.hpp"
#include "..\scripts\srctas_reader.hpp"
#include "..\scripts\tests\test.hpp"
#include "..\..\utils\game_detection.hpp"
#include "..\..\aim.hpp"
#include "bspflags.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

using std::size_t;
using std::uintptr_t;

void __fastcall ClientDLL::HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_CViewRender__OnRenderStart_Func(thisptr, edx);
}

#define DEF_FUTURE(name) auto f##name = FindAsync(ORIG_##name, patterns::client::##name);
#define GET_HOOKEDFUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[client dll] Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			patternContainer.AddHook(HOOKED_##future_name, (PVOID*)&ORIG_##future_name); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::client::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning("[client dll] Could not find " #future_name ".\n"); \
		} \
	}

#define GET_FUTURE(future_name) \
	{ \
		auto pattern = f##future_name.get(); \
		if (ORIG_##future_name) \
		{ \
			DevMsg("[client dll] Found " #future_name " at %p (using the %s pattern).\n", \
			       ORIG_##future_name, \
			       pattern->name()); \
			for (int i = 0; true; ++i) \
			{ \
				if (patterns::client::##future_name.at(i).name() == pattern->name()) \
				{ \
					patternContainer.AddIndex((PVOID*)&ORIG_##future_name, i, pattern->name()); \
					break; \
				} \
			} \
		} \
		else \
		{ \
			DevWarning("[client dll] Could not find " #future_name ".\n"); \
		} \
	}

void ClientDLL::Hook(const std::wstring& moduleName,
                     void* moduleHandle,
                     void* moduleBase,
                     size_t moduleLength,
                     bool needToIntercept)
{
	Clear(); // Just in case.
	m_Name = moduleName;
	m_Base = moduleBase;
	m_Length = moduleLength;
	uintptr_t ORIG_MiddleOfCAM_Think, ORIG_CHLClient__CanRecordDemo, ORIG_CHudDamageIndicator__GetDamagePosition;

	patternContainer.Init(moduleName);

	DEF_FUTURE(CViewRender__OnRenderStart);
	DEF_FUTURE(MiddleOfCAM_Think);
	DEF_FUTURE(CHLClient__CanRecordDemo);
	DEF_FUTURE(UTIL_TraceRay);
	DEF_FUTURE(CGameMovement__CanUnDuckJump);
	DEF_FUTURE(CHudDamageIndicator__GetDamagePosition);

	GET_HOOKEDFUTURE(CViewRender__OnRenderStart);
	GET_FUTURE(CHLClient__CanRecordDemo);
	GET_FUTURE(UTIL_TraceRay);
	GET_FUTURE(CGameMovement__CanUnDuckJump);
	GET_FUTURE(CHudDamageIndicator__GetDamagePosition);

	if (ORIG_CHLClient__CanRecordDemo)
	{
		int offset = *reinterpret_cast<int*>(ORIG_CHLClient__CanRecordDemo + 1);
		ORIG_GetClientModeNormal = (_GetClientModeNormal)(offset + ORIG_CHLClient__CanRecordDemo + 5);
		DevMsg("[client.dll] Found GetClientModeNormal at %p\n", ORIG_GetClientModeNormal);
	}

	if (ORIG_CHudDamageIndicator__GetDamagePosition)
	{
		int offset = *reinterpret_cast<int*>(ORIG_CHudDamageIndicator__GetDamagePosition + 4);
		ORIG_MainViewOrigin = (_MainViewOrigin)(offset + ORIG_CHudDamageIndicator__GetDamagePosition + 8);
		DevMsg("[client.dll] Found MainViewOrigin at %p\n", ORIG_MainViewOrigin);
	}

	if (!ORIG_CViewRender__OnRenderStart)
	{
		Warning("_y_spt_force_fov has no effect.\n");
	}

	if (!ORIG_UTIL_TraceRay)
		Warning("tas_strafe_version 1 not available\n");

	if (!ORIG_MainViewOrigin || !ORIG_UTIL_TraceRay)
		Warning("y_spt_hud_oob 1 has no effect\n");

	patternContainer.Hook();
}

void ClientDLL::Unhook()
{
	patternContainer.Unhook();
	Clear();
}

void ClientDLL::Clear()
{
	IHookableNameFilter::Clear();
	ORIG_UTIL_TraceRay = nullptr;
	ORIG_MainViewOrigin = nullptr;
}

Vector ClientDLL::GetCameraOrigin()
{
	if (!ORIG_MainViewOrigin)
		return Vector();
	return ORIG_MainViewOrigin();
}


bool ClientDLL::CanUnDuckJump(trace_t& ptr)
{
	if (!ORIG_CGameMovement__CanUnDuckJump)
	{
		Warning("Tried to run CanUnduckJump without the pattern!\n");
		return false;
	}

	return ORIG_CGameMovement__CanUnDuckJump(GetGamemovement(), 0, ptr);
}

void __fastcall ClientDLL::HOOKED_CViewRender__OnRenderStart_Func(void* thisptr, int edx)
{
	ORIG_CViewRender__OnRenderStart(thisptr, edx);

	if (!_viewmodel_fov || !_y_spt_force_fov.GetBool())
		return;

	float* fov = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisptr) + 52);
	float* fovViewmodel = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisptr) + 56);
	*fov = _y_spt_force_fov.GetFloat();
	*fovViewmodel = _viewmodel_fov->GetFloat();
}
