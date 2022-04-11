#pragma once
#include "../feature.hpp"
#include "tier0/basetypes.h"

// Gets you rect
class RectProvider : public FeatureWrapper<RectProvider>
{
public:
	vrect_t GetRect();
	virtual bool ShouldLoadFeature() override;
protected:
	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
};

extern RectProvider spt_rectprovider;