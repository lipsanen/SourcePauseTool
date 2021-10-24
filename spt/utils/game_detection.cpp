#include "stdafx.h"
#include "..\OrangeBox\spt-serverplugin.hpp"
#include "SPTLib\sptlib.hpp"

namespace utils
{
	bool DoesGameLookLikePortal()
	{
#ifndef OE
		if (g_pCVar)
		{
			if (g_pCVar->FindCommand("upgrade_portalgun"))
				return true;

			return false;
		}

		auto engine = GetEngineClient();

		if (engine)
		{
			auto game_dir = engine->GetGameDirectory();
			return (GetFileName(Convert(game_dir)) == L"portal");
		}
#endif

		return false;
	}

	bool DoesGameLookLikeDMoMM()
	{
#ifdef OE
		auto* g_pCVar = GetCvarInterface();
		if (g_pCVar)
		{
			if (g_pCVar->FindVar("mm_xana_fov"))
				return true;
		}
#endif

		return false;
	}

	bool DoesGameLookLikeHLS()
	{
		auto* g_pCVar = GetCvarInterface();
		if (g_pCVar)
		{
			if (g_pCVar->FindVar("hl1_ref_db_distance"))
				return true;
		}

		return false;
	}
} // namespace utils