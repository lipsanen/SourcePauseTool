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

typedef void(__cdecl* _DoImageSpaceMotionBlur)(void* view, int x, int y, int w, int h);
typedef bool(__fastcall* _CheckJumpButton)(void* thisptr, int edx);
typedef void(__fastcall* _CViewRender__OnRenderStart)(void* thisptr, int edx);
typedef void(
    __fastcall* _CViewRender__RenderView)(void* thisptr, int edx, void* cameraView, int nClearFlags, int whatToDraw);
typedef void(__fastcall* _CViewRender__Render)(void* thisptr, int edx, void* rect);
typedef void*(__cdecl* _GetClientModeNormal)();
typedef void(__cdecl* _UTIL_TraceLine)(const Vector& vecAbsStart,
                                       const Vector& vecAbsEnd,
                                       unsigned int mask,
                                       ITraceFilter* pFilter,
                                       trace_t* ptr);
typedef void(__fastcall* _UTIL_TracePlayerBBox)(void* thisptr,
                                                int edx,
                                                const Vector& vecAbsStart,
                                                const Vector& vecAbsEnd,
                                                unsigned int mask,
                                                int collisionGroup,
                                                trace_t& ptr);
typedef void(__cdecl* _UTIL_TraceRay)(const Ray_t& ray,
                                      unsigned int mask,
                                      const IHandleEntity* ignore,
                                      int collisionGroup,
                                      trace_t* ptr);
typedef bool(__fastcall* _CGameMovement__CanUnDuckJump)(void* thisptr, int edx, trace_t& ptr);
typedef void(__fastcall* _CViewEffects__Fade)(void* thisptr, int edx, void* data);
typedef void(__fastcall* _CViewEffects__Shake)(void* thisptr, int edx, void* data);
typedef const Vector&(__cdecl* _MainViewOrigin)();
typedef void(__cdecl* _ResetToneMapping)(float value);


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

	static void __cdecl HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h);
	static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	static void __stdcall HOOKED_HudUpdate(bool bActive);
	static void __fastcall HOOKED_CreateMove(void* thisptr,
	                                         int edx,
	                                         int sequence_number,
	                                         float input_sample_frametime,
	                                         bool active);
	static void __fastcall HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx);
	static void __fastcall HOOKED_CViewRender__RenderView(void* thisptr,
	                                                      int edx,
	                                                      void* cameraView,
	                                                      int nClearFlags,
	                                                      int whatToDraw);
	static void __fastcall HOOKED_CViewRender__Render(void* thisptr, int edx, void* rect);
	static void __fastcall HOOKED_CViewEffects__Fade(void* thisptr, int edx, void* data);
	static void __fastcall HOOKED_CViewEffects__Shake(void* thisptr, int edx, void* data);
	void __cdecl HOOKED_DoImageSpaceMotionBlur_Func(void* view, int x, int y, int w, int h);
	bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);
	void __fastcall HOOKED_CViewRender__OnRenderStart_Func(void* thisptr, int edx);
	void __fastcall HOOKED_CViewRender__RenderView_Func(void* thisptr,
	                                                    int edx,
	                                                    void* cameraView,
	                                                    int nClearFlags,
	                                                    int whatToDraw);
	void __fastcall HOOKED_CViewRender__Render_Func(void* thisptr, int edx, void* rect);
	static void __cdecl HOOKED_ResetToneMapping(float value);

	Vector GetCameraOrigin();
	bool CanUnDuckJump(trace_t& ptr);

	bool renderingOverlay;
	void* screenRect;
	_GetClientModeNormal ORIG_GetClientModeNormal;
	_UTIL_TraceRay ORIG_UTIL_TraceRay;

protected:
	_DoImageSpaceMotionBlur ORIG_DoImageSpaceMotionBlur;
	_CheckJumpButton ORIG_CheckJumpButton;
	_CViewRender__OnRenderStart ORIG_CViewRender__OnRenderStart;
	_CViewRender__RenderView ORIG_CViewRender__RenderView;
	_CViewRender__Render ORIG_CViewRender__Render;
	_CGameMovement__CanUnDuckJump ORIG_CGameMovement__CanUnDuckJump;
	_CViewEffects__Fade ORIG_CViewEffects__Fade;
	_CViewEffects__Shake ORIG_CViewEffects__Shake;
	_MainViewOrigin ORIG_MainViewOrigin;
	_ResetToneMapping ORIG_ResetToneMapping;

	uintptr_t* pgpGlobals;
	ptrdiff_t off1M_nOldButtons;
	ptrdiff_t off2M_nOldButtons;

protected:
	bool cantJumpNextTime;
	PatternContainer patternContainer;
};
