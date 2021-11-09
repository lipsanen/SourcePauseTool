#include "stdafx.h"

#include "..\OrangeBox\modules.hpp"
#include "..\features\generic.hpp"
#include "tier1\tier1.h"
#include "tier2\tier2.h"
#include "tier3\tier3.h"
#include "vgui_utils.hpp"

namespace vgui
{
#ifndef OE
	IClientMode* GetClientMode()
	{
		if(generic_.ORIG_GetClientModeNormal)
			return (IClientMode*)generic_.ORIG_GetClientModeNormal();
		else
			return nullptr;
	}
	IScheme* GetScheme()
	{
		return scheme()->GetIScheme(scheme()->GetDefaultScheme());
	}
#endif
} // namespace vgui