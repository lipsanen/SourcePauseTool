#include "..\..\stdafx.hpp"
#pragma once

#include <vector>

#include <SPTLib\IHookableNameFilter.hpp>
#include "..\spt-serverplugin.hpp"
#include "..\..\SDK\igamemovement.h"
#include "SDK/../../public/cdll_int.h"

using std::uintptr_t;
using std::size_t;

typedef void(__cdecl *_DoImageSpaceMotionBlur) (void* view, int x, int y, int w, int h);
typedef bool(__fastcall *_CheckJumpButton) (void* thisptr, int edx);
typedef void(__stdcall *_HudUpdate) (bool bActive);
typedef int(__fastcall *_GetButtonBits) (void* thisptr, int edx, int bResetState);
typedef void(__fastcall *_AdjustAngles) (void* thisptr, int edx, float frametime);
typedef void(__fastcall *_CreateMove) (void* thisptr, int edx, int sequence_number, float input_sample_frametime, bool active);
typedef void(__fastcall *_CViewRender__OnRenderStart) (void* thisptr, int edx);
typedef void*(__cdecl *_GetLocalPlayer) ();
typedef void*(__fastcall *_GetGroundEntity) (void* thisptr, int edx);
typedef void(__fastcall *_CalcAbsoluteVelocity) (void* thisptr, int edx);


typedef void(__fastcall * _CViewRender__ViewDrawScene) (void * thisptr, int edx, int bDrew3dSkybox, int skyboxVis, void * view, int nClearFlags, int viewID, int drawViewModel, int baseDrawFlags, void * customVis);
typedef bool(__fastcall * _CViewRender__DrawOneMonitor) (void * thisptr, int edx, void * pRenderTarget, int cameraNum, void * cameraEnt, void * cameraView, void * localPlayer, int x, int y, int width, int height);
typedef void(__fastcall * _CViewRender__DrawMonitors) (void * thisptr, int edx, void * cameraView);
typedef void(__fastcall * _CPortalRenderable_FlatBasic__RenderPortalViewToTexture) (void * thisptr, int edx, void * cviewrender, void * cameraView);
typedef void(__fastcall * _CViewRender__QueueOverlayRenderView) (void * thisptr, int edx, void * view, int nClearFlags, int whatToDraw);

typedef struct
{
	long long int framesLeft;
	std::string command;
} afterframes_entry_t;

typedef struct 
{
	float angle;
	bool set;
} angset_command_t;

class ClientDLL : public IHookableNameFilter
{
public:
	ClientDLL() : IHookableNameFilter({ L"client.dll" }) {};
	virtual void Hook(const std::wstring& moduleName, HMODULE hModule, uintptr_t moduleStart, size_t moduleLength);
	virtual void Unhook();
	virtual void Clear();

	static void __cdecl HOOKED_DoImageSpaceMotionBlur(void* view, int x, int y, int w, int h);
	static bool __fastcall HOOKED_CheckJumpButton(void* thisptr, int edx);
	static void __stdcall HOOKED_HudUpdate(bool bActive);
	static int __fastcall HOOKED_GetButtonBits(void* thisptr, int edx, int bResetState);
	static void __fastcall HOOKED_AdjustAngles(void* thisptr, int edx, float frametime);
	static void __fastcall HOOKED_CreateMove (void* thisptr, int edx, int sequence_number, float input_sample_frametime, bool active);
	static void __fastcall HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx);
	static bool __fastcall HOOKED_CViewRender__DrawOneMonitor(void * thisptr, int edx, void * pRenderTarget, int cameraNum, void * cameraEnt, void * cameraView, void * localPlayer, int x, int y, int width, int height);
	static void __fastcall HOOKED_CViewRender__ViewDrawScene(void * thisptr, int edx, bool bDrew3dSkybox, int skyboxVis, void * view, int nClearFlags, int viewID, bool drawViewModel, int baseDrawFlags, void * customVis);
	static void __fastcall HOOKED_CViewRender__DrawMonitors(void * thisptr, int edx, void * cameraView);
	static void __fastcall HOOKED_CPortalRenderable_FlatBasic__RenderPortalViewToTexture(void * thisptr, int edx, void * cviewrender, void * cameraView);
	static void __fastcall HOOKED_CViewRender__QueueOverlayRenderView(void * thisptr, int edx, void * view, int nClearFlags, int whatToDraw);

	void __cdecl HOOKED_DoImageSpaceMotionBlur_Func(void* view, int x, int y, int w, int h);
	bool __fastcall HOOKED_CheckJumpButton_Func(void* thisptr, int edx);
	void __stdcall HOOKED_HudUpdate_Func(bool bActive);
	int __fastcall HOOKED_GetButtonBits_Func(void* thisptr, int edx, int bResetState);
	void __fastcall HOOKED_AdjustAngles_Func(void* thisptr, int edx, float frametime);
	void __fastcall HOOKED_CreateMove_Func(void* thisptr, int edx, int sequence_number, float input_sample_frametime, bool active);
	void __fastcall HOOKED_CViewRender__OnRenderStart_Func(void* thisptr, int edx);
	bool __fastcall HOOKED_CViewRender__DrawOneMonitor_Func(void * thisptr, int edx, void * pRenderTarget, int cameraNum, void * cameraEnt, void * cameraView, void * localPlayer, int x, int y, int width, int height);
	void __fastcall HOOKED_CViewRender__ViewDrawScene_Func(void * thisptr, int edx, bool bDrew3dSkybox, int skyboxVis, void * view, int nClearFlags, int viewID, bool drawViewModel, int baseDrawFlags, void * customVis);
	void __fastcall HOOKED_CViewRender__DrawMonitors_Func(void * thisptr, int edx, void * cameraView);
	void __fastcall HOOKED_CPortalRenderable_FlatBasic__RenderPortalViewToTexture_Func(void * thisptr, int edx, void * cviewrender, void * cameraView);
	void __fastcall HOOKED_CViewRender__QueueOverlayRenderView_Func(void * thisptr, int edx, void * view, int nClearFlags, int whatToDraw);

	void ViewDrawScene(bool bDrew3dSkybox, int skyboxVis, void * view, int nClearFlags);
	void DelayAfterframesQueue(int delay);
	void AddIntoAfterframesQueue(const afterframes_entry_t& entry);
	void ResetAfterframesQueue();

	void PauseAfterframesQueue() { afterframesPaused = true; }
	void ResumeAfterframesQueue() { afterframesPaused = false; }

	void EnableDuckspam()  { duckspam = true; }
	void DisableDuckspam() { duckspam = false; }

	void SetPitch(float pitch) { setPitch.angle = pitch; setPitch.set = true; }
	void SetYaw(float yaw)     { setYaw.angle   = yaw;   setYaw.set   = true; }
	Vector GetPlayerVelocity();
	Vector GetPlayerEyePos();
	bool GetFlagsDucking();

protected:
	_DoImageSpaceMotionBlur ORIG_DoImageSpaceMotionBlur;
	_CheckJumpButton ORIG_CheckJumpButton;
	_HudUpdate ORIG_HudUpdate;
	_GetButtonBits ORIG_GetButtonBits;
	_AdjustAngles ORIG_AdjustAngles;
	_CreateMove ORIG_CreateMove;
	_CViewRender__OnRenderStart ORIG_CViewRender__OnRenderStart;
	_GetLocalPlayer GetLocalPlayer;
	_GetGroundEntity GetGroundEntity;
	_CalcAbsoluteVelocity CalcAbsoluteVelocity;
	_CViewRender__DrawOneMonitor ORIG_CViewRender__DrawOneMonitor;
	_CViewRender__ViewDrawScene ORIG_CViewRender__ViewDrawScene;
	_CViewRender__DrawMonitors ORIG_CViewRender__DrawMonitors;
	_CPortalRenderable_FlatBasic__RenderPortalViewToTexture ORIG_CPortalRenderable_FlatBasic__RenderPortalViewToTexture;
	_CViewRender__QueueOverlayRenderView ORIG_CViewRender__QueueOverlayRenderView;

	uintptr_t* pgpGlobals;
	ptrdiff_t offM_pCommands;
	ptrdiff_t off1M_nOldButtons;
	ptrdiff_t off2M_nOldButtons;
	ptrdiff_t offForwardmove;
	ptrdiff_t offSidemove;
	ptrdiff_t offMaxspeed;
	ptrdiff_t offFlags;
	ptrdiff_t offAbsVelocity;
	ptrdiff_t offDucking;
	ptrdiff_t offDuckJumpTime;
	ptrdiff_t offServerSurfaceFriction;
	ptrdiff_t offServerPreviouslyPredictedOrigin;
public:
	ptrdiff_t offServerAbsOrigin;
protected:
	uintptr_t pCmd;

	bool tasAddressesWereFound;

	std::vector<afterframes_entry_t> afterframesQueue;
	bool afterframesPaused = false;

	bool duckspam;
	angset_command_t setPitch, setYaw;
	bool forceJump;
	bool cantJumpNextTime;

	void OnFrame();

	int afterframesDelay;
	void * clientCViewRender;
};
