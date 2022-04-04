#include "stdafx.h"
#include "../feature.hpp"
#include "../utils/interfaces.hpp"
#include "convar.hpp"

ConVar y_spt_focus_nosleep("y_spt_focus_nosleep", "0", 0, "Improves FPS while alt-tabbed.");

// Gives the option to disable sleeping to improve FPS while alt-tabbed
class NoSleepFeature : public FeatureWrapper<NoSleepFeature>
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	DECL_DETOUR(void, CInputSystem__SleepUntilInput, int nMaxSleepTimeMS);
};

static NoSleepFeature spt_nosleep;

bool NoSleepFeature::ShouldLoadFeature()
{
	return true;
}

void NoSleepFeature::InitHooks()
{
	HOOK_VTABLE(inputsystem, interfaces::inputSystem, &IInputSystem::SleepUntilInput, CInputSystem__SleepUntilInput);
}

void NoSleepFeature::LoadFeature()
{
	if (ORIG_CInputSystem__SleepUntilInput)
	{
		InitConcommandBase(y_spt_focus_nosleep);
	}
}

void NoSleepFeature::UnloadFeature() {}

DETOUR(void, NoSleepFeature, CInputSystem__SleepUntilInput, int nMaxSleepTimeMS)
{
	if (y_spt_focus_nosleep.GetBool())
		nMaxSleepTimeMS = 0;
	spt_nosleep.ORIG_CInputSystem__SleepUntilInput(thisptr, nMaxSleepTimeMS);
}
