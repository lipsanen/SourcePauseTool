#pragma once

#include <functional>
#include <vector>
#include "../feature.hpp"
#include "vgui/VGUI.h"
#include "vgui/IScheme.h"
#include "VGuiMatSurface/IMatSystemSurface.h"

enum class RenderTime
{
	overlay,
	paint
};

struct HudCallback
{
	HudCallback(std::string key, std::function<void()> draw, std::function<bool()> shouldDraw, bool overlay);

	RenderTime renderTime;
	std::string sortKey;
	std::function<void()> draw;
	std::function<bool()> shouldDraw;
};

// HUD stuff
class HUDFeature : public FeatureWrapper<HUDFeature>
{
public:
	bool AddHudCallback(HudCallback callback);
	void DrawTopHudElement(const wchar* format, ...);
	virtual bool ShouldLoadFeature() override;

	IMatSystemSurface* surface = nullptr;
	vgui::IScheme* scheme = nullptr;
	vgui::HFont font = 0;

protected:
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	virtual void PreHook() override;
	virtual void UnloadFeature() override;

private:
	void DrawHUD();

	DECL_MEMBER(void, FinishDrawing);
	DECL_MEMBER(void, StartDrawing);
	DECL_DETOUR(void, VGui_Paint, int mode);

	bool callbacksSorted = true;
	std::vector<HudCallback> hudCallbacks;

	int topX = 0;
	int topVertIndex = 0;

	ConVar* cl_showpos = nullptr;
	ConVar* cl_showfps = nullptr;
	int topFontTall = 0;
	bool loadingSuccessful = false;
};

extern HUDFeature spt_hud;
