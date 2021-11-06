#pragma once
#include "..\feature.hpp"

typedef int(__fastcall* DemoPlayer__Func)(void* thisptr);
typedef void(__fastcall* _StopRecording)(void* thisptr, int edx);
typedef void(__fastcall* _SetSignonState)(void* thisptr, int edx, int state);
typedef void(__cdecl* _Stop)();

// Various demo features
class DemoStuff : public Feature
{
public:
	void Demo_StopRecording();
	int Demo_GetPlaybackTick() const;
	int Demo_GetTotalTicks() const;
	bool Demo_IsPlayingBack() const;
	bool Demo_IsPlaybackPaused() const;
	bool Demo_IsAutoRecordingAvailable() const;

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

private:
	int GetPlaybackTick_Offset;
	int GetTotalTicks_Offset;
	int IsPlayingBack_Offset;
	int IsPlaybackPaused_Offset;
	void** pDemoplayer;
	int currentAutoRecordDemoNumber;
	int m_nDemoNumber_Offset;
	int m_bRecording_Offset;
	bool isAutoRecordingDemo;

	_StopRecording ORIG_StopRecording;
	_SetSignonState ORIG_SetSignonState;
	_Stop ORIG_Stop;
	uintptr_t ORIG_Record;

	static void __fastcall HOOKED_StopRecording(void* thisptr, int edx);
	static void __fastcall HOOKED_SetSignonState(void* thisptr, int edx, int state);
	static void __cdecl HOOKED_Stop();
};

extern DemoStuff g_Demostuff;
