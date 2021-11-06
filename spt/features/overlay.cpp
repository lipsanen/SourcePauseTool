#include "stdafx.h"
#ifdef SSDK2007
#include "overlay.hpp"
#include "..\OrangeBox\overlay\overlay-renderer.hpp"

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