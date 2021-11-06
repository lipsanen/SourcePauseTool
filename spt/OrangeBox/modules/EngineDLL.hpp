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

typedef void(__cdecl* __Host_RunFrame)(float time);
typedef void(__cdecl* __Host_RunFrame_Input)(float accumulated_extra_samples, int bFinalTick);
typedef void(__cdecl* __Host_RunFrame_Server)(int bFinalTick);
typedef void(__cdecl* _Cbuf_Execute)();
typedef int(__fastcall* DemoPlayer__Func)(void* thisptr);
typedef bool(__fastcall* _CEngineTrace__PointOutsideWorld)(void* thisptr, int edx, const Vector& pt);
typedef void(__cdecl* _Host_AccumulateTime)(float dt);
typedef void(__fastcall* _StopRecording)(void* thisptr, int edx);
typedef void(__fastcall* _SetSignonState)(void* thisptr, int edx, int state);
typedef void(__cdecl* _Stop)();

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

	static void __cdecl HOOKED__Host_RunFrame(float time);
	static void __cdecl HOOKED__Host_RunFrame_Input(float accumulated_extra_samples, int bFinalTick);
	static void __cdecl HOOKED__Host_RunFrame_Server(int bFinalTick);
	static void __cdecl HOOKED_Host_AccumulateTime(float dt);
	static void __fastcall HOOKED_StopRecording(void* thisptr, int edx);
	static void __fastcall HOOKED_SetSignonState(void* thisptr, int edx, int state);
	static void __cdecl HOOKED_Cbuf_Execute();
	static void __cdecl HOOKED_Stop();
	void __cdecl HOOKED__Host_RunFrame_Func(float time);
	void __cdecl HOOKED__Host_RunFrame_Input_Func(float accumulated_extra_samples, int bFinalTick);
	void __cdecl HOOKED__Host_RunFrame_Server_Func(int bFinalTick);
	void __cdecl HOOKED_Cbuf_Execute_Func();
	void __fastcall HOOKED_StopRecording_Func(void* thisptr, int edx);
	void __fastcall HOOKED_SetSignonState_Func(void* thisptr, int edx, int state);
	void __cdecl HOOKED_Stop_Func();

	float GetTickrate() const;
	void SetTickrate(float value);

	int Demo_GetPlaybackTick() const;
	int Demo_GetTotalTicks() const;
	bool Demo_IsPlayingBack() const;
	bool Demo_IsPlaybackPaused() const;
	void Demo_StopRecording();
	bool Demo_IsAutoRecordingAvailable() const;
	_CEngineTrace__PointOutsideWorld ORIG_CEngineTrace__PointOutsideWorld;

protected:
	PatternContainer patternContainer;
	__Host_RunFrame ORIG__Host_RunFrame;
	__Host_RunFrame_Input ORIG__Host_RunFrame_Input;
	__Host_RunFrame_Server ORIG__Host_RunFrame_Server;
	_Cbuf_Execute ORIG_Cbuf_Execute;
	_Host_AccumulateTime ORIG_Host_AccumulateTime;
	_StopRecording ORIG_StopRecording;
	_SetSignonState ORIG_SetSignonState;
	_Stop ORIG_Stop;

	void* pGameServer;
	bool* pM_bLoadgame;
	bool shouldPreventNextUnpause;
	float* pIntervalPerTick;
	float* pHost_Frametime;
	float* pHost_Realtime;
	int* pM_State;
	int* pM_nSignonState;
	void** pDemoplayer;
	int currentAutoRecordDemoNumber;
	bool isAutoRecordingDemo;

	int GetPlaybackTick_Offset;
	int GetTotalTicks_Offset;
	int IsPlayingBack_Offset;
	int IsPlaybackPaused_Offset;

	int m_nDemoNumber_Offset;
	int m_bRecording_Offset;
};
