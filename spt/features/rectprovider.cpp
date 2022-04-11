#include "stdafx.h"
#include "rectprovider.hpp"
#include "interfaces.hpp"

RectProvider spt_rectprovider;

bool RectProvider::ShouldLoadFeature()
{
	return interfaces::clientInterface != nullptr;
}

void RectProvider::InitHooks() {}

void RectProvider::LoadFeature() {}

void RectProvider::UnloadFeature() {}

vrect_t RectProvider::GetRect()
{
	vrect_t rect;
	rect.pnext = nullptr;
	rect.height = interfaces::clientInterface->GetScreenHeight();
	rect.width = interfaces::clientInterface->GetScreenWidth();
	rect.x = 0;
	rect.y = 0;

	return rect;
}