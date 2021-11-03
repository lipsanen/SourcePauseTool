#pragma once
#include "..\feature.hpp"
#include "thirdparty\Signal.h"

// For hooks used by many features
class GenericFeature : public Feature
{
public:
	Gallant::Signal0<void> TickSignal;
	Gallant::Signal1<bool> OngroundSignal;

	void Tick();

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;
};

extern GenericFeature generic_;
