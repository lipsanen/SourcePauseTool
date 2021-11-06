#include <chrono>
#include <functional>
#include <sstream>
#include <time.h>

#include <SPTLib\Hooks.hpp>
#include "spt-serverplugin.hpp"
#include "..\sptlib-wrapper.hpp"
#include "..\utils\ent_utils.hpp"
#include "..\utils\math.hpp"
#include "..\utils\string_parsing.hpp"
#include "..\utils\game_detection.hpp"
#include "..\features\generic.hpp"
#include "..\features\playerio.hpp"
#include "custom_interfaces.hpp"
#include "cvars.hpp"
#include "modules.hpp"
#include "scripts\srctas_reader.hpp"
#include "scripts\tests\test.hpp"
#include "..\ipc\ipc-spt.hpp"
#include "..\feature.hpp"
#include "vstdlib\random.h"

#include "cdll_int.h"
#include "eiface.h"
#include "engine\iserverplugin.h"
#include "icliententitylist.h"
#include "tier2\tier2.h"
#include "tier3\tier3.h"
#include "vgui\iinput.h"
#include "vgui\isystem.h"
#include "vgui\ivgui.h"

#if SSDK2007
#include "mathlib\vmatrix.h"
#endif

#include "..\vgui\vgui_utils.hpp"
#include "SPTLib\sptlib.hpp"
#include "module_hooks.hpp"
#include "overlay\overlay-renderer.hpp"
#include "overlay\overlays.hpp"
#include "tier0\memdbgoff.h" // YaLTeR - switch off the memory debugging.
using namespace std::literals;

// useful helper func
inline bool FStrEq(const char* sz1, const char* sz2)
{
	return (Q_stricmp(sz1, sz2) == 0);
}

// Interfaces from the engine
std::unique_ptr<EngineClientWrapper> engine;
IVEngineServer* engine_server = nullptr;
IMatSystemSurface* surface = nullptr;
vgui::ISchemeManager* scheme = nullptr;
IVDebugOverlay* debugOverlay = nullptr;
void* gm = nullptr;

int lastSeed = 0;

// For OE CVar and ConCommand registering.
#if defined(OE)
class CPluginConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase* pCommand)
	{
		pCommand->AddFlags(FCVAR_PLUGIN);

		// Unlink from plugin only list
		pCommand->SetNext(0);

		// Link to engine's list instead
		g_pCVar->RegisterConCommandBase(pCommand);
		return true;
	}
};

CPluginConVarAccessor g_ConVarAccessor;

// OE: For correct linking in VS 2015.
int(WINAPIV* __vsnprintf)(char*, size_t, const char*, va_list) = _vsnprintf;
#endif

#ifdef OE
ArgsWrapper::ArgsWrapper()
{
	this->engine_pointer = engine.get();
}
#endif

void CallServerCommand(const char* cmd)
{
	if (engine)
		engine->ClientCmd(cmd);
}

void GetViewAngles(float viewangles[3])
{
	if (engine)
	{
		QAngle va;
		engine->GetViewAngles(va);

		viewangles[0] = va.x;
		viewangles[1] = va.y;
		viewangles[2] = va.z;
	}
}

void SetViewAngles(const float viewangles[3])
{
	if (engine)
	{
		QAngle va(viewangles[0], viewangles[1], viewangles[2]);
		engine->SetViewAngles(va);
	}
}

void DefaultFOVChangeCallback(ConVar* var, char const* pOldString)
{
	if (FStrEq(var->GetString(), "75") && FStrEq(pOldString, "90"))
	{
		//Msg("Attempted to change default_fov from 90 to 75. Preventing.\n");
		var->SetValue("90");
	}
}

IVEngineServer* GetEngine()
{
	return engine_server;
}

IVDebugOverlay* GetDebugOverlay()
{
	return debugOverlay;
}

void* GetGamemovement()
{
	return gm;
}

IMatSystemSurface* GetSurface()
{
	return surface;
}

vgui::IScheme* GetIScheme()
{
	return scheme->GetIScheme(scheme->GetDefaultScheme());
}

ICvar* GetCvarInterface()
{
	return g_pCVar;
}

std::string GetGameDir()
{
#ifdef OE
	return std::string(y_spt_gamedir.GetString());
#else
	return engine->GetGameDirectory();
#endif
}

EngineClientWrapper* GetEngineClient()
{
	return engine.get();
}

#ifndef P2
IServerUnknown* GetServerPlayer()
{
	if (!engine_server)
		return nullptr;

	auto edict = engine_server->PEntityOfEntIndex(1);
	if (!edict)
		return nullptr;

	return edict->GetUnknown();
}
#else
// TODO
IServerUnknown* GetServerPlayer()
{
	assert(false);
	return nullptr;
}
#endif

bool FoundEngineServer()
{
	return (engine_server != nullptr);
}

//
// The plugin is a static singleton that is exported as an interface
//
CSourcePauseTool g_SourcePauseTool;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CSourcePauseTool,
                                  IServerPluginCallbacks,
                                  INTERFACEVERSION_ISERVERPLUGINCALLBACKS,
                                  g_SourcePauseTool);

bool CSourcePauseTool::Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory)
{
	auto startTime = std::chrono::high_resolution_clock::now();

	ConnectTier1Libraries(&interfaceFactory, 1);
	ConnectTier3Libraries(&interfaceFactory, 1);

	gm = gameServerFactory(INTERFACENAME_GAMEMOVEMENT, NULL);
	if (gm)
	{
		DevMsg("SPT: Found IGameMovement at %p.\n", gm);
	}
	else
	{
		DevWarning("SPT: Could not find IGameMovement.\n");
		DevWarning("SPT: ProcessMovement logging with tas_log is unavailable.\n");
	}

	if (!g_pCVar)
	{
		DevWarning("SPT: Failed to get the ICvar interface.\n");
		Warning("SPT: Could not register any CVars and ConCommands.\n");
		Warning("SPT: y_spt_cvar has no effect.\n");
	}
#if defined(OE)
	else
	{
		ConCommandBaseMgr::OneTimeInit(&g_ConVarAccessor);

		_viewmodel_fov = g_pCVar->FindVar("viewmodel_fov");
		if (!_viewmodel_fov)
		{
			DevWarning("SPT: Could not find viewmodel_fov.\n");
			Warning("SPT: _y_spt_force_fov has no effect.\n");
		}
	}
#else
	else
	{
#define GETCVAR(x) _##x = g_pCVar->FindVar(#x);

		GETCVAR(sv_airaccelerate);
		GETCVAR(sv_accelerate);
		GETCVAR(sv_friction);
		GETCVAR(sv_maxspeed);
		GETCVAR(sv_stopspeed);
		GETCVAR(sv_stepsize);
		GETCVAR(sv_gravity);
		GETCVAR(sv_maxvelocity);
		GETCVAR(sv_bounce);
	}

	ConVar_Register(0);
#endif

#if !defined(P2) && !defined(BMS)
	auto ptr = interfaceFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
#else
	auto ptr = interfaceFactory("VEngineClient015", NULL);
#endif

	if (ptr)
	{
#ifdef OE
		if (utils::DoesGameLookLikeDMoMM())
			engine = std::make_unique<IVEngineClientWrapper<IVEngineClientDMoMM>>(
			    reinterpret_cast<IVEngineClientDMoMM*>(ptr));
#endif

		// Check if we assigned it in the ifdef above.
		if (!engine)
			engine = std::make_unique<IVEngineClientWrapper<IVEngineClient>>(
			    reinterpret_cast<IVEngineClient*>(ptr));
	}

	if (!engine)
	{
		DevWarning("SPT: Failed to get the IVEngineClient interface.\n");
		Warning("SPT: y_spt_afterframes has no effect.\n");
		Warning("SPT: _y_spt_setpitch and _y_spt_setyaw have no effect.\n");
		Warning("SPT: _y_spt_pitchspeed and _y_spt_yawspeed have no effect.\n");
		Warning("SPT: y_spt_stucksave has no effect.\n");
	}

	if (utils::DoesGameLookLikePortal())
	{
		DevMsg("SPT: This game looks like portal. Setting the tas_* cvars appropriately.\n");

		tas_force_airaccelerate.SetValue(15);
		tas_force_wishspeed_cap.SetValue(60);
		tas_reset_surface_friction.SetValue(0);
	}

	engine_server = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
	debugOverlay = (IVDebugOverlay*)interfaceFactory(VDEBUG_OVERLAY_INTERFACE_VERSION, NULL);
	if (!engine_server)
	{
		DevWarning("SPT: Failed to get the IVEngineServer interface.\n");
	}

	if (!debugOverlay)
	{
		DevWarning("SPT: Failed to get the debug overlay interface.\n");
		Warning("Seam visualization has no effect.\n");
	}

#ifndef OE
	auto clientFactory = Sys_GetFactory("client");
	IClientEntityList* entList = (IClientEntityList*)clientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, NULL);
	IVModelInfo* modelInfo = (IVModelInfo*)interfaceFactory(VMODELINFO_SERVER_INTERFACE_VERSION, NULL);
	IBaseClientDLL* clientInterface = (IBaseClientDLL*)clientFactory(CLIENT_DLL_INTERFACE_VERSION, NULL);

	if (entList)
	{
		utils::SetEntityList(entList);
	}
	else
		DevWarning("Unable to retrieve entitylist interface.\n");

	if (modelInfo)
	{
		utils::SetModelInfo(modelInfo);
	}
	else
		DevWarning("Unable to retrieve the model info interface.\n");

	if (clientInterface)
	{
		utils::SetClientDLL(clientInterface);
	}
	else
		DevWarning("Unable to retrieve the client DLL interface.\n");

#endif

	EngineConCmd = CallServerCommand;
	EngineGetViewAngles = GetViewAngles;
	EngineSetViewAngles = SetViewAngles;

	_EngineMsg = Msg;
	_EngineDevMsg = DevMsg;
	_EngineWarning = Warning;
	_EngineDevWarning = DevWarning;

	Hooks::AddToHookedModules(&clientDLL);
	Hooks::AddToHookedModules(&serverDLL);
	Hooks::Init(true);
	Feature::LoadFeatures();
	ipc::Init();
	ModuleHooks::ConnectSignals();

	auto loadTime =
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - startTime)
	        .count();
	std::ostringstream out;
	out << "SourcePauseTool version " SPT_VERSION " was loaded in " << loadTime << "ms.\n";

	Msg("%s", std::string(out.str()).c_str());

	return true;
}

void CSourcePauseTool::Unload(void)
{
	Hooks::Free();

#if !defined(OE)
	ConVar_Unregister();
#endif

	DisconnectTier1Libraries();
	DisconnectTier3Libraries();
	ipc::ShutdownIPC();
}

const char* CSourcePauseTool::GetPluginDescription(void)
{
	return "SourcePauseTool v" SPT_VERSION ", Ivan \"YaLTeR\" Molodetskikh";
}

CON_COMMAND(y_spt_cvar, "CVar manipulation.")
{
	if (!engine || !g_pCVar)
		return;

#if defined(OE)
	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_cvar <name> [value]\n");
		return;
	}

	ConVar* cvar = g_pCVar->FindVar(args.Arg(1));
	if (!cvar)
	{
		Warning("Couldn't find the cvar: %s\n", args.Arg(1));
		return;
	}

	if (args.ArgC() == 2)
	{
		Msg("\"%s\" = \"%s\"\n", cvar->GetName(), cvar->GetString(), cvar->GetHelpText());
		Msg("Default: %s\n", cvar->GetDefault());

		float val;
		if (cvar->GetMin(val))
		{
			Msg("Min: %f\n", val);
		}

		if (cvar->GetMax(val))
		{
			Msg("Max: %f\n", val);
		}

		const char* helpText = cvar->GetHelpText();
		if (helpText[0] != '\0')
			Msg("- %s\n", cvar->GetHelpText());

		return;
	}

	const char* value = args.Arg(2);
	cvar->SetValue(value);
}

CON_COMMAND(y_spt_cvar_random, "Randomize CVar value.")
{
	if (!engine || !g_pCVar)
		return;

#if defined(OE)
	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() != 4)
	{
		Msg("Usage: y_spt_cvar_random <name> <min> <max>\n");
		return;
	}

	ConVar* cvar = g_pCVar->FindVar(args.Arg(1));
	if (!cvar)
	{
		Warning("Couldn't find the cvar: %s\n", args.Arg(1));
		return;
	}

	float min = std::stof(args.Arg(2));
	float max = std::stof(args.Arg(3));

	float r = utils::RandomFloat(min, max);
	cvar->SetValue(r);
}

#if !defined(OE)
CON_COMMAND(y_spt_cvar2, "CVar manipulation, sets the CVar value to the rest of the argument string.")
{
	if (!engine || !g_pCVar)
		return;

	if (args.ArgC() < 2)
	{
		Msg("Usage: y_spt_cvar <name> [value]\n");
		return;
	}

	ConVar* cvar = g_pCVar->FindVar(args.Arg(1));
	if (!cvar)
	{
		Warning("Couldn't find the cvar: %s\n", args.Arg(1));
		return;
	}

	if (args.ArgC() == 2)
	{
		Msg("\"%s\" = \"%s\"\n", cvar->GetName(), cvar->GetString(), cvar->GetHelpText());
		Msg("Default: %s\n", cvar->GetDefault());

		float val;
		if (cvar->GetMin(val))
		{
			Msg("Min: %f\n", val);
		}

		if (cvar->GetMax(val))
		{
			Msg("Max: %f\n", val);
		}

		const char* helpText = cvar->GetHelpText();
		if (helpText[0] != '\0')
			Msg("- %s\n", cvar->GetHelpText());

		return;
	}

	const char* value = args.ArgS() + strlen(args.Arg(1)) + 1;
	cvar->SetValue(value);
}
#endif

CON_COMMAND(y_spt_canjb, "Tests if player can jumpbug on a given height, with the current position and speed.")
{
	if (!engine)
		return;

#if defined(OE)
	ArgsWrapper args(engine.get());
#endif

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

#endif

#ifndef OE

CON_COMMAND(y_spt_print_ent_props, "Prints all props for a given entity index.")
{
#if defined(OE)
	ArgsWrapper args(engine.get());
#endif
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

CON_COMMAND(_y_spt_getangles, "Gets the view angles of the player.")
{
	if (!engine)
		return;

	QAngle va;
	engine->GetViewAngles(va);

	Warning("View Angle (x): %f\n", va.x);
	Warning("View Angle (y): %f\n", va.y);
	Warning("View Angle (z): %f\n", va.z);
	Warning("View Angle (x, y, z): %f %f %f\n", va.x, va.y, va.z);
}

CON_COMMAND(y_spt_timer_start, "Starts the SPT timer.")
{
	serverDLL.StartTimer();
}

CON_COMMAND(y_spt_timer_stop, "Stops the SPT timer and prints the current time.")
{
	serverDLL.StopTimer();
	Warning("Current time (in ticks): %u\n", serverDLL.GetTicksPassed());
}

CON_COMMAND(y_spt_timer_reset, "Stops and resets the SPT timer.")
{
	serverDLL.ResetTimer();
}

CON_COMMAND(y_spt_timer_print, "Prints the current time of the SPT timer.")
{
	Warning("Current time (in ticks): %u\n", serverDLL.GetTicksPassed());
}

CON_COMMAND(
    tas_script_load,
    "Loads and executes an .srctas script. If an extra ticks argument is given, the script is played back at maximal FPS and without rendering until that many ticks before the end of the script. Usage: tas_load_script [script] [ticks]")
{
#if defined(OE)
	if (!engine)
		return;

	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() == 2)
		scripts::g_TASReader.ExecuteScript(args.Arg(1));
	else if (args.ArgC() == 3)
	{
		scripts::g_TASReader.ExecuteScriptWithResume(args.Arg(1), std::stoi(args.Arg(2)));
	}
	else
		Msg("Loads and executes an .srctas script. Usage: tas_load_script [script]\n");
}

CON_COMMAND(tas_script_search, "Starts a variable search for an .srctas script. Usage: tas_load_search [script]")
{
#if defined(OE)
	if (!engine)
		return;

	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() > 1)
		scripts::g_TASReader.StartSearch(args.Arg(1));
	else
		Msg("Starts a variable search for an .srctas script. Usage: tas_load_search [script]\n");
}

CON_COMMAND(tas_script_result_success, "Signals a successful result in a variable search.")
{
	scripts::g_TASReader.SearchResult(scripts::SearchResult::Success);
}

CON_COMMAND(tas_script_result_fail, "Signals an unsuccessful result in a variable search.")
{
	scripts::g_TASReader.SearchResult(scripts::SearchResult::Fail);
}

CON_COMMAND(tas_script_result_stop, "Signals a stop in a variable search.")
{
	scripts::g_TASReader.SearchResult(scripts::SearchResult::NoSearch);
}

CON_COMMAND(tas_test_generate, "Generates test data for given test.")
{
#if defined(OE)
	if (!engine)
		return;

	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() > 1)
	{
		scripts::g_Tester.LoadTest(args.Arg(1), true);
	}
}

CON_COMMAND(tas_test_validate, "Validates a test.")
{
#if defined(OE)
	if (!engine)
		return;

	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() > 1)
	{
		scripts::g_Tester.LoadTest(args.Arg(1), false);
	}
}

CON_COMMAND(tas_test_automated_validate, "Validates a test, produces a log file and exits the game.")
{
#if defined(OE)
	if (!engine)
		return;

	ArgsWrapper args(engine.get());
#endif

	if (args.ArgC() > 2)
	{
		scripts::g_Tester.RunAutomatedTest(args.Arg(1), false, args.Arg(2));
	}
}

CON_COMMAND(tas_print_movement_vars, "Prints movement vars.")
{
	auto vars = playerio::GetMovementVars();

	Msg("Accelerate %f\n", vars.Accelerate);
	Msg("AirAccelerate %f\n", vars.Airaccelerate);
	Msg("Bounce %f\n", vars.Bounce);
	Msg("EntFriction %f\n", vars.EntFriction);
	Msg("EntGrav %f\n", vars.EntGravity);
	Msg("Frametime %f\n", vars.Frametime);
	Msg("Friction %f\n", vars.Friction);
	Msg("Grav %f\n", vars.Gravity);
	Msg("Maxspeed %f\n", vars.Maxspeed);
	Msg("Maxvelocity %f\n", vars.Maxvelocity);
	Msg("OnGround %d\n", vars.OnGround);
	Msg("ReduceWishspeed %d\n", vars.ReduceWishspeed);
	Msg("Stepsize %f\n", vars.Stepsize);
	Msg("Stopspeed %f\n", vars.Stopspeed);
	Msg("WS cap %f\n", vars.WishspeedCap);
}

#if SSDK2007
// TODO: remove fixed offsets.

void setang_exact(const QAngle& angles)
{
	auto player = GetServerPlayer();
	auto teleport = reinterpret_cast<void(__fastcall*)(void*, int, const Vector*, const QAngle*, const Vector*)>(
	    (*reinterpret_cast<uintptr_t**>(player))[105]);

	teleport(player, 0, nullptr, &angles, nullptr);
	serverDLL.SnapEyeAngles(player, 0, angles);
}

// Trace as if we were firing a Portal with the given viewangles, return the squared distance to the resulting point and the normal.
double trace_fire_portal(QAngle angles, Vector& normal)
{
	setang_exact(angles);

	serverDLL.FirePortal(serverDLL.GetActiveWeapon(GetServerPlayer()), 0, false, nullptr, true);

	normal = serverDLL.lastPortalTrace.plane.normal;
	return (serverDLL.lastPortalTrace.endpos - serverDLL.lastPortalTrace.startpos).LengthSqr();
}

QAngle firstAngle;
bool firstInvocation = true;

CON_COMMAND(
    y_spt_find_seam_shot,
    "y_spt_find_seam_shot [<pitch1> <yaw1> <pitch2> <yaw2> <epsilon>] - tries to find a seam shot on a \"line\" between viewangles (pitch1; yaw1) and (pitch2; yaw2) with binary search. Decreasing epsilon will result in more viewangles checked. A default value is 0.00001. If no arguments are given, first invocation selects the first point, second invocation selects the second point and searches between them.")
{
	QAngle a, b;
	double eps = 0.00001 * 0.00001;

	if (args.ArgC() == 1)
	{
		if (firstInvocation)
		{
			engine->GetViewAngles(firstAngle);
			firstInvocation = !firstInvocation;

			Msg("First point set.\n");
			return;
		}
		else
		{
			firstInvocation = !firstInvocation;

			a = firstAngle;
			engine->GetViewAngles(b);
		}
	}
	else
	{
		if (args.ArgC() != 5 && args.ArgC() != 6)
		{
			Msg("Usage: y_spt_find_seam_shot <pitch1> <yaw1> <pitch2> <yaw2> <epsilon> - tries to find a seam shot on a \"line\" between viewangles (pitch1; yaw1) and (pitch2; yaw2) with binary search. Decreasing epsilon will result in more viewangles checked. A default value is 0.00001. If no arguments are given, first invocation selects the first point, second invocation selects the second point and searches between them.\n");
			return;
		}

		a = QAngle(atof(args.Arg(1)), atof(args.Arg(2)), 0);
		b = QAngle(atof(args.Arg(3)), atof(args.Arg(4)), 0);
		eps = (args.ArgC() == 5) ? eps : std::pow(atof(args.Arg(5)), 2);
	}

	if (!serverDLL.GetActiveWeapon(GetServerPlayer()))
	{
		Msg("You need to be holding a portal gun.\n");
		return;
	}

	Vector a_normal;
	const auto distance = std::max(trace_fire_portal(a, a_normal), trace_fire_portal(b, Vector()));

	// If our trace had a distance greater than the a or b distance by this amount, treat it as a seam shot.
	constexpr double GOOD_DISTANCE_DIFFERENCE = 50.0 * 50.0;

	QAngle test;
	utils::GetMiddlePoint(a, b, test);
	double difference;

	do
	{
		Vector test_normal;

		if (trace_fire_portal(test, test_normal) - distance > GOOD_DISTANCE_DIFFERENCE)
		{
			Msg("Found a seam shot at setang %.8f %.8f 0\n", test.x, test.y);
			return;
		}

		if (test_normal == a_normal)
		{
			a = test;
			utils::GetMiddlePoint(a, b, test);
			difference = (test - a).LengthSqr();
		}
		else
		{
			b = test;
			utils::GetMiddlePoint(a, b, test);
			difference = (test - b).LengthSqr();
		}

		Msg("Difference: %.8f\n", std::sqrt(difference));
	} while (difference > eps);

	Msg("Could not find a seam shot. Best guess: setang %.8f %.8f 0\n", test.x, test.y);
}

#endif
