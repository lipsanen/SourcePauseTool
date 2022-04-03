#pragma once

#ifndef OE

class IClientMode;

#include "inputsystem/ButtonCode.h"
#include "vgui/IScheme.h"
#include "vgui_controls/Controls.h"

namespace vgui
{
	IClientMode* GetClientMode();
	IScheme* GetScheme();
} // namespace vgui

#endif
