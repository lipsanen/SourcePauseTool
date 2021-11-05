#pragma once
#include "..\feature.hpp"
#include "thirdparty\Signal.h"

typedef void(__stdcall* _HudUpdate)(bool bActive);

// For hooks used by many features
class GenericFeature : public Feature
{
public:
	Gallant::Signal0<void> TickSignal;
	Gallant::Signal1<bool> OngroundSignal;
	Gallant::Signal0<void> FrameSignal;
	_HudUpdate ORIG_HudUpdate;

	void Tick();

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;
private:
	static void __stdcall HOOKED_HudUpdate(bool bActive);
};

extern GenericFeature generic_;
