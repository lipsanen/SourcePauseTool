#include "stdafx.h"
#ifdef SSDK2007
#include "game_detection.hpp"
#include "tracing.hpp"
#include "signals.hpp"
#include "interfaces.hpp"
#include "..\feature.hpp"
#include "..\vgui\lines.hpp"

class GraphicsFeature : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void LoadFeature() override;
};

static GraphicsFeature spt_graphics;

bool GraphicsFeature::ShouldLoadFeature()
{
	return utils::DoesGameLookLikePortal() && interfaces::debugOverlay;
}

void GraphicsFeature::LoadFeature()
{
	if (spt_tracing.ORIG_FirePortal && interfaces::debugOverlay)
	{
		AdjustAngles.Connect(vgui::DrawLines);
	}
}

#endif