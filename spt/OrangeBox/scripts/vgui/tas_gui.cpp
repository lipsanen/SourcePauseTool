#include "stdafx.h"
#include "tas_gui.hpp"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include "iclientmode.h"

using namespace vgui;

namespace scripts
{
	IClientMode* ic;
	CTASGUI* pTASGUI = nullptr;

	CTASGUI::CTASGUI(const char* panelName) : Panel(NULL, panelName, true)
	{
	}

	CTASGUI::~CTASGUI()
	{
		pTASGUI = nullptr;
	}

	void OpenTASGUI()
	{		
	}

}