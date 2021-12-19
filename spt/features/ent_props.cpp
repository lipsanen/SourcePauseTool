#include "stdafx.h"
#include "..\feature.hpp"
#include "SPTLib\patterns.hpp"
#include "convar.hpp"
#include "hud.hpp"
#include "playerio.hpp"
#include "game_detection.hpp"
#include "interfaces.hpp"
#include "ent_utils.hpp"
#include "string_utils.hpp"
#include "..\overlay\portal_camera.hpp"
#include <map>
#include <string>

ConVar y_spt_hud_portal_bubble("y_spt_hud_portal_bubble", "0", FCVAR_CHEAT, "Turns on portal bubble index hud.\n");
ConVar y_spt_hud_ent_info(
    "y_spt_hud_ent_info",
    "",
FCVAR_CHEAT,
"Display entity info on HUD. Format is \"[ent index],[prop regex],[prop regex],...,[prop regex];[ent index],...,[prop regex]\".\n");

static const int MAX_ENTRIES = 128;
static const int INFO_BUFFER_SIZE = 256;
static wchar INFO_ARRAY[MAX_ENTRIES * INFO_BUFFER_SIZE];
static const char ENT_SEPARATOR = ';';
static const char PROP_SEPARATOR = ',';

namespace patterns
{
	PATTERNS(
		Datamap,
		"pattern1",
		"C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? B8",
		"pattern2",
		"C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? C3");
}

// Initializes ent utils stuff
class EntUtils : public Feature
{
public:
	virtual bool ShouldLoadFeature()
	{
#ifdef OE
		return false;
#else
		return true;
#endif
	}

	virtual void LoadFeature() override;
	virtual void InitHooks() override;
	virtual void PreHook() override;

protected:
	std::vector<uintptr_t> serverPatterns;
	std::vector<uintptr_t> clientPatterns;

	std::map<std::string, datamap_t*> serverMaps;
	std::map<std::string, datamap_t*> clientMaps;
};

static EntUtils spt_entutils;

void EntUtils::InitHooks()
{
	AddMultiPatternHook(patterns::Datamap, "client", "Datamap", &clientPatterns);
	AddMultiPatternHook(patterns::Datamap, "server", "Datamap", &serverPatterns);
}

static bool IsAddressLegal(uint8_t* address, uint8_t* start, std::size_t length)
{
	return address >= start && address <= start + length;
}

static bool DoesNameLookSane(datamap_t* map, uint8_t* moduleStart, std::size_t length)
{
	int MAX_LEN = 64;
	char* name = const_cast<char*>(map->dataClassName);

	if (!IsAddressLegal(reinterpret_cast<uint8_t*>(name), moduleStart, length))
		return false;

	for (int i = 0; i < MAX_LEN; ++i)
	{
		if (name[i] == '\0')
		{
			return i > 0;
		}
	}

	return false;
}

void EntUtils::PreHook()
{
	void* clhandle;
	void* clmoduleStart;
	size_t clmoduleSize;
	void* svhandle;
	void* svmoduleStart;
	size_t svmoduleSize;

	if (MemUtils::GetModuleInfo(L"server.dll", &svhandle, &svmoduleStart, &svmoduleSize))
	{
		for (auto& serverPattern : serverPatterns)
		{
			int numfields = *reinterpret_cast<int*>(serverPattern + 6);
			datamap_t** pmap = reinterpret_cast<datamap_t**>(serverPattern + 12);
			if (numfields > 0
				&& IsAddressLegal(reinterpret_cast<uint8_t*>(pmap),
					reinterpret_cast<uint8_t*>(svmoduleStart),
					svmoduleSize))
			{
				datamap_t* map = *pmap;
				if (DoesNameLookSane(map, reinterpret_cast<uint8_t*>(svmoduleStart), svmoduleSize))
					serverMaps[map->dataClassName] = map;
			}
		}
	}

	if (MemUtils::GetModuleInfo(L"client.dll", &clhandle, &clmoduleStart, &clmoduleSize))
	{
		for (auto& clientPattern : clientPatterns)
		{
			int numfields = *reinterpret_cast<int*>(clientPattern + 6);
			datamap_t** pmap = reinterpret_cast<datamap_t**>(clientPattern + 12);
			if (numfields > 0
			    && IsAddressLegal(reinterpret_cast<uint8_t*>(pmap),
			                      reinterpret_cast<uint8_t*>(clmoduleStart),
			                      clmoduleSize))
			{
				datamap_t* map = *pmap;
				if (DoesNameLookSane(map, reinterpret_cast<uint8_t*>(clmoduleStart), clmoduleSize))
					clientMaps[map->dataClassName] = map;
			}
		}
	}

	clientPatterns.clear();
	serverPatterns.clear();
}

CON_COMMAND(y_spt_canjb, "Tests if player can jumpbug on a given height, with the current position and speed.")
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_canjb [height]\n");
		return;
	}

	float height = std::stof(args.Arg(1));
	auto can = utils::CanJB(height);

	if (can.canJB)
		Msg("Yes, projected landing height %.6f in %d ticks\n", can.landingHeight - height, can.ticks);
	else
		Msg("No, missed by %.6f in %d ticks.\n", can.landingHeight - height, can.ticks);
}

#ifndef OE
CON_COMMAND(y_spt_print_ents, "Prints all client entity indices and their corresponding classes.")
{
	utils::PrintAllClientEntities();
}

CON_COMMAND(y_spt_print_portals, "Prints all portal indexes, their position and angles.")
{
	utils::PrintAllPortals();
}

CON_COMMAND(y_spt_print_ent_props, "Prints all props for a given entity index.")
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_print_ent_props [index]\n");
	}
	else
	{
		utils::PrintAllProps(std::stoi(args.Arg(1)));
	}
}
#endif

void EntUtils::LoadFeature()
{
#ifndef OE
	InitCommand(y_spt_canjb);
	InitCommand(y_spt_print_ents);
	InitCommand(y_spt_print_ent_props);
	if (utils::DoesGameLookLikePortal())
		InitCommand(y_spt_print_portals);
#endif
#ifdef SSDK2007
	if (utils::DoesGameLookLikePortal())
	{
		AddHudCallback(
		    "portal bubble",
		    [this]() {
			    int in_bubble = GetEnvironmentPortal() != NULL;
			    spt_hud.DrawTopHudElement(L"portal bubble: %d", in_bubble);
		    },
		    y_spt_hud_portal_bubble);

		bool result = spt_hud.AddHudCallback(HudCallback(
		    "z",
		    []() {
			    std::string info(y_spt_hud_ent_info.GetString());
			    if (!whiteSpacesOnly(info))
			    {
				    int entries = utils::FillInfoArray(info,
				                                       INFO_ARRAY,
				                                       MAX_ENTRIES,
				                                       INFO_BUFFER_SIZE,
				                                       PROP_SEPARATOR,
				                                       ENT_SEPARATOR);
				    for (int i = 0; i < entries; ++i)
				    {
					    spt_hud.DrawTopHudElement(INFO_ARRAY + i * INFO_BUFFER_SIZE);
				    }
			    }
		    },
		    []() { return true; },
		    false));

		if (result)
		{
			InitConcommandBase(y_spt_hud_ent_info);
		}
	}
#endif
}
