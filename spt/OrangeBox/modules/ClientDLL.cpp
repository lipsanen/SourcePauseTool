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

void __cdecl ClientDLL::HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_DoImageSpaceMotionBlur_Func(view, x, y, w, h);
}

bool __fastcall ClientDLL::HOOKED_CheckJumpButton(void* thisptr, int edx)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_CheckJumpButton_Func(thisptr, edx);
}

void __stdcall ClientDLL::HOOKED_HudUpdate(bool bActive)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_HudUpdate_Func(bActive);
}

void __fastcall ClientDLL::HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx)
{
	TRACE_ENTER();
	return clientDLL.HOOKED_CViewRender__OnRenderStart_Func(thisptr, edx);
}

void ClientDLL::HOOKED_CViewRender__RenderView(void* thisptr,
                                               int edx,
                                               void* cameraView,
                                               int nClearFlags,
                                               int whatToDraw)
{
	TRACE_ENTER();
	clientDLL.HOOKED_CViewRender__RenderView_Func(thisptr, edx, cameraView, nClearFlags, whatToDraw);
}

void ClientDLL::HOOKED_CViewRender__Render(void* thisptr, int edx, void* rect)
{
	TRACE_ENTER();
	clientDLL.HOOKED_CViewRender__Render_Func(thisptr, edx, rect);
}

ConVar y_spt_disable_fade("y_spt_disable_fade", "0", FCVAR_ARCHIVE, "Disables all fades.");

void __fastcall ClientDLL::HOOKED_CViewEffects__Fade(void* thisptr, int edx, void* data)
{
	if (!y_spt_disable_fade.GetBool())
		clientDLL.ORIG_CViewEffects__Fade(thisptr, edx, data);
}

ConVar y_spt_disable_shake("y_spt_disable_shake", "0", FCVAR_ARCHIVE, "Disables all shakes.");

void __fastcall ClientDLL::HOOKED_CViewEffects__Shake(void* thisptr, int edx, void* data)
{
	if (!y_spt_disable_shake.GetBool())
		clientDLL.ORIG_CViewEffects__Shake(thisptr, edx, data);
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

	DEF_FUTURE(HudUpdate);
	DEF_FUTURE(CViewRender__OnRenderStart);
	DEF_FUTURE(MiddleOfCAM_Think);
	DEF_FUTURE(DoImageSpaceMotionBlur);
	DEF_FUTURE(CheckJumpButton);
	DEF_FUTURE(CHLClient__CanRecordDemo);
	DEF_FUTURE(CViewRender__RenderView);
	DEF_FUTURE(CViewRender__Render);
	DEF_FUTURE(UTIL_TraceRay);
	DEF_FUTURE(CGameMovement__CanUnDuckJump);
	DEF_FUTURE(CViewEffects__Fade);
	DEF_FUTURE(CViewEffects__Shake);
	DEF_FUTURE(CHudDamageIndicator__GetDamagePosition);
	DEF_FUTURE(ResetToneMapping);

	GET_HOOKEDFUTURE(HudUpdate);
	GET_HOOKEDFUTURE(CViewRender__OnRenderStart);
	GET_HOOKEDFUTURE(DoImageSpaceMotionBlur);
	GET_HOOKEDFUTURE(CheckJumpButton);
	GET_FUTURE(CHLClient__CanRecordDemo);
	GET_HOOKEDFUTURE(CViewRender__RenderView);
	GET_HOOKEDFUTURE(CViewRender__Render);
	GET_FUTURE(UTIL_TraceRay);
	GET_FUTURE(CGameMovement__CanUnDuckJump);
	GET_HOOKEDFUTURE(CViewEffects__Fade);
	GET_HOOKEDFUTURE(CViewEffects__Shake);
	GET_FUTURE(CHudDamageIndicator__GetDamagePosition);
	GET_HOOKEDFUTURE(ResetToneMapping);

	if (ORIG_DoImageSpaceMotionBlur)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_DoImageSpaceMotionBlur);

		switch (ptnNumber)
		{
		case 0:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 132);
			break;

		case 1:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 153);
			break;

		case 2:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 129);
			break;

		case 3:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 171);
			break;

		case 4:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 177);
			break;

		case 5:
			pgpGlobals = *(uintptr_t**)((uintptr_t)ORIG_DoImageSpaceMotionBlur + 128);
			break;
		}

		DevMsg("[client dll] pgpGlobals is %p.\n", pgpGlobals);
	}
	else
	{
		Warning("y_spt_motion_blur_fix has no effect.\n");
	}

	if (ORIG_CheckJumpButton)
	{
		int ptnNumber = patternContainer.FindPatternIndex((PVOID*)&ORIG_CheckJumpButton);

		switch (ptnNumber)
		{
		case 0:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;
		case 1:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 2:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 3:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 4:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 5:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 6:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 7:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 8:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
			break;

		case 9:
			off1M_nOldButtons = 1;
			off2M_nOldButtons = 40;
			break;

		case 10:
			off1M_nOldButtons = 2;
			off2M_nOldButtons = 40;
		}
	}
	else
	{
		Warning("y_spt_autojump has no effect in multiplayer.\n");
	}

	if (!ORIG_HudUpdate)
	{
		Warning("_y_spt_afterframes has no effect.\n");
	}

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

	if (ORIG_CViewRender__RenderView == nullptr || ORIG_CViewRender__Render == nullptr)
		Warning("Overlay cameras have no effect.\n");

	if (!ORIG_UTIL_TraceRay)
		Warning("tas_strafe_version 1 not available\n");

	if (!ORIG_CViewEffects__Fade)
		Warning("y_spt_disable_fade 1 not available\n");

	if (!ORIG_CViewEffects__Shake)
		Warning("y_spt_disable_shake 1 not available\n");

	if (!ORIG_MainViewOrigin || !ORIG_UTIL_TraceRay)
		Warning("y_spt_hud_oob 1 has no effect\n");

	if (!ORIG_ResetToneMapping)
		Warning("y_spt_disable_tone_map_reset has no effect\n");

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
	ORIG_DoImageSpaceMotionBlur = nullptr;
	ORIG_CheckJumpButton = nullptr;
	ORIG_HudUpdate = nullptr;
	ORIG_CViewRender__RenderView = nullptr;
	ORIG_CViewRender__Render = nullptr;
	ORIG_UTIL_TraceRay = nullptr;
	ORIG_MainViewOrigin = nullptr;
	ORIG_ResetToneMapping = nullptr;

	pgpGlobals = nullptr;
	off1M_nOldButtons = 0;
	off2M_nOldButtons = 0;

	afterframesQueue.clear();
	afterframesPaused = false;
}

void ClientDLL::DelayAfterframesQueue(int delay)
{
	afterframesDelay = delay;
}

void ClientDLL::AddIntoAfterframesQueue(const afterframes_entry_t& entry)
{
	afterframesQueue.push_back(entry);
}

void ClientDLL::ResetAfterframesQueue()
{
	afterframesQueue.clear();
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

void ClientDLL::OnFrame()
{
	FrameSignal();

	if (afterframesPaused)
	{
		return;
	}

	if (afterframesDelay-- > 0)
	{
		return;
	}

	for (auto it = afterframesQueue.begin(); it != afterframesQueue.end();)
	{
		it->framesLeft--;
		if (it->framesLeft <= 0)
		{
			EngineConCmd(it->command.c_str());
			it = afterframesQueue.erase(it);
		}
		else
			++it;
	}

	AfterFramesSignal();
}

void __cdecl ClientDLL::HOOKED_DoImageSpaceMotionBlur_Func(void* view, int x, int y, int w, int h)
{
	uintptr_t origgpGlobals = NULL;

	/*
	Replace gpGlobals with (gpGlobals + 12). gpGlobals->realtime is the first variable,
	so it is located at gpGlobals. (gpGlobals + 12) is gpGlobals->curtime. This
	function does not use anything apart from gpGlobals->realtime from gpGlobals,
	so we can do such a replace to make it use gpGlobals->curtime instead without
	breaking anything else.
	*/
	if (pgpGlobals)
	{
		if (y_spt_motion_blur_fix.GetBool())
		{
			origgpGlobals = *pgpGlobals;
			*pgpGlobals = *pgpGlobals + 12;
		}
	}

	ORIG_DoImageSpaceMotionBlur(view, x, y, w, h);

	if (pgpGlobals)
	{
		if (y_spt_motion_blur_fix.GetBool())
		{
			*pgpGlobals = origgpGlobals;
		}
	}
}

bool __fastcall ClientDLL::HOOKED_CheckJumpButton_Func(void* thisptr, int edx)
{
	/*
	Not sure if this gets called at all from the client dll, but
	I will just hook it in exactly the same way as the server one.
	*/
	const int IN_JUMP = (1 << 1);

	int* pM_nOldButtons = NULL;
	int origM_nOldButtons = 0;

	if (y_spt_autojump.GetBool())
	{
		pM_nOldButtons = (int*)(*((uintptr_t*)thisptr + off1M_nOldButtons) + off2M_nOldButtons);
		origM_nOldButtons = *pM_nOldButtons;

		if (!cantJumpNextTime) // Do not do anything if we jumped on the previous tick.
		{
			*pM_nOldButtons &= ~IN_JUMP; // Reset the jump button state as if it wasn't pressed.
		}
		else
		{
			// DevMsg( "Con jump prevented!\n" );
		}
	}

	cantJumpNextTime = false;

	bool rv = ORIG_CheckJumpButton(thisptr, edx); // This function can only change the jump bit.

	if (y_spt_autojump.GetBool())
	{
		if (!(*pM_nOldButtons & IN_JUMP)) // CheckJumpButton didn't change anything (we didn't jump).
		{
			*pM_nOldButtons = origM_nOldButtons; // Restore the old jump button state.
		}
	}

	if (rv)
	{
		// We jumped.
		if (_y_spt_autojump_ensure_legit.GetBool())
		{
			cantJumpNextTime = true; // Prevent consecutive jumps.
		}

		// DevMsg( "Jump!\n" );
	}

	DevMsg("Engine call: [client dll] CheckJumpButton() => %s\n", (rv ? "true" : "false"));

	return rv;
}

void __stdcall ClientDLL::HOOKED_HudUpdate_Func(bool bActive)
{
	OnFrame();

	return ORIG_HudUpdate(bActive);
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

void ClientDLL::HOOKED_CViewRender__RenderView_Func(void* thisptr,
                                                    int edx,
                                                    void* cameraView,
                                                    int nClearFlags,
                                                    int whatToDraw)
{
#ifndef SSDK2007
	ORIG_CViewRender__RenderView(thisptr, edx, cameraView, nClearFlags, whatToDraw);
#else
	if (g_OverlayRenderer.shouldRenderOverlay())
	{
		g_OverlayRenderer.modifyView(static_cast<CViewSetup*>(cameraView), renderingOverlay);
		if (renderingOverlay)
		{
			g_OverlayRenderer.modifySmallScreenFlags(nClearFlags, whatToDraw);
		}
		else
		{
			g_OverlayRenderer.modifyBigScreenFlags(nClearFlags, whatToDraw);
		}
	}

	ORIG_CViewRender__RenderView(thisptr, edx, cameraView, nClearFlags, whatToDraw);
#endif
}

void ClientDLL::HOOKED_CViewRender__Render_Func(void* thisptr, int edx, void* rect)
{
#ifndef SSDK2007
	ORIG_CViewRender__Render(thisptr, edx, rect);
#else
	renderingOverlay = false;
	screenRect = rect;
	if (!g_OverlayRenderer.shouldRenderOverlay())
	{
		ORIG_CViewRender__Render(thisptr, edx, rect);
	}
	else
	{
		ORIG_CViewRender__Render(thisptr, edx, rect);

		renderingOverlay = true;
		Rect_t rec = g_OverlayRenderer.getRect();
		screenRect = &rec;
		ORIG_CViewRender__Render(thisptr, edx, &rec);
		renderingOverlay = false;
	}
#endif
}

void ClientDLL::HOOKED_ResetToneMapping(float value)
{
	if (!y_spt_disable_tone_map_reset.GetBool())
		clientDLL.ORIG_ResetToneMapping(value);
}
