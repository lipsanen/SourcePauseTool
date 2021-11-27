#include "stdafx.h"
#ifdef SSDK2007
#include "overlay.hpp"
#include "..\overlay\overlay-renderer.hpp"

ConVar _y_spt_overlay("_y_spt_overlay",
	"0",
	FCVAR_CHEAT,
	"Enables the overlay camera in the top left of the screen.\n");
ConVar _y_spt_overlay_type(
	"_y_spt_overlay_type",
	"0",
	FCVAR_CHEAT,
	"Overlay type. 0 = save glitch body, 1 = angle glitch simulation, 2 = rear view mirror, 3 = havok view mirror.\n");
ConVar _y_spt_overlay_portal(
	"_y_spt_overlay_portal",
	"auto",
	FCVAR_CHEAT,
	"Chooses the portal for the overlay camera. Valid options are blue/orange/portal index. For the SG camera this is the portal you save glitch on, for angle glitch simulation this is the portal you enter.\n");
ConVar _y_spt_overlay_width("_y_spt_overlay_width",
	"480",
	FCVAR_CHEAT,
	"Determines the width of the overlay. Height is determined automatically from width.\n",
	true,
	20.0f,
	true,
	3840.0f);
ConVar _y_spt_overlay_fov("_y_spt_overlay_fov",
	"90",
	FCVAR_CHEAT,
	"Determines the FOV of the overlay.\n",
	true,
	5.0f,
	true,
	140.0f);
ConVar _y_spt_overlay_swap("_y_spt_overlay_swap", "0", FCVAR_CHEAT, "Swap alternate view and main screen?\n");


Overlay g_Overlay;

bool Overlay::ShouldLoadFeature()
{
	return true;
}

void Overlay::InitHooks()
{
	HOOK_FUNCTION(client, CViewRender__Render);
	HOOK_FUNCTION(client, CViewRender__RenderView);
}

void Overlay::LoadFeature()
{
	if (ORIG_CViewRender__RenderView == nullptr || ORIG_CViewRender__Render == nullptr)
		Warning("Overlay cameras have no effect.\n");
}

void Overlay::UnloadFeature() {}

void __fastcall Overlay::HOOKED_CViewRender__RenderView(void* thisptr,
                                                        int edx,
                                                        void* cameraView,
                                                        int nClearFlags,
                                                        int whatToDraw)
{
	if (g_Overlay.ORIG_CViewRender__RenderView == nullptr || g_Overlay.ORIG_CViewRender__Render == nullptr)
	{
		g_Overlay.ORIG_CViewRender__RenderView(thisptr, edx, cameraView, nClearFlags, whatToDraw);
	}
	else
	{
		if (g_OverlayRenderer.shouldRenderOverlay())
		{
			g_OverlayRenderer.modifyView(static_cast<CViewSetup*>(cameraView), g_Overlay.renderingOverlay);
			if (g_Overlay.renderingOverlay)
			{
				g_OverlayRenderer.modifySmallScreenFlags(nClearFlags, whatToDraw);
			}
			else
			{
				g_OverlayRenderer.modifyBigScreenFlags(nClearFlags, whatToDraw);
			}
		}

		g_Overlay.ORIG_CViewRender__RenderView(thisptr, edx, cameraView, nClearFlags, whatToDraw);
	}
}

void __fastcall Overlay::HOOKED_CViewRender__Render(void* thisptr, int edx, void* rect)
{
	if (g_Overlay.ORIG_CViewRender__RenderView == nullptr || g_Overlay.ORIG_CViewRender__Render == nullptr)
	{
		g_Overlay.ORIG_CViewRender__Render(thisptr, edx, rect);
	}
	else
	{
		g_Overlay.renderingOverlay = false;
		g_Overlay.screenRect = rect;
		if (!g_OverlayRenderer.shouldRenderOverlay())
		{
			g_Overlay.ORIG_CViewRender__Render(thisptr, edx, rect);
		}
		else
		{
			g_Overlay.ORIG_CViewRender__Render(thisptr, edx, rect);

			g_Overlay.renderingOverlay = true;
			Rect_t rec = g_OverlayRenderer.getRect();
			g_Overlay.screenRect = &rec;
			g_Overlay.ORIG_CViewRender__Render(thisptr, edx, &rec);
			g_Overlay.renderingOverlay = false;
		}
	}
}

#endif