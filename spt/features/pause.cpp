#include "stdafx.h"
#include "..\feature.hpp"
#include "generic.hpp"
#include "convar.h"
#include "dbg.h"

ConVar y_spt_pause("y_spt_pause", "0", FCVAR_ARCHIVE);

// Feature description
class PauseFeature : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
private:
	uintptr_t ORIG_SpawnPlayer;
	bool* pM_bLoadgame;
	void* pGameServer;

	void SV_ActivateServer(bool result);
	void FinishRestore(void* thisptr, int edx);
	void SetPaused(void* thisptr, int edx, bool paused);
};

static PauseFeature _pause;

bool PauseFeature::ShouldLoadFeature()
{
	return true;
}

void PauseFeature::InitHooks()
{
	FIND_PATTERN(engine, SpawnPlayer);
}

void PauseFeature::LoadFeature()
{
	pM_bLoadgame = nullptr;
	pGameServer = nullptr;
	generic_.SV_ActivateServerSignal.Connect(this, &PauseFeature::SV_ActivateServer);
	generic_.SetPausedSignal.Connect(this, &PauseFeature::SetPaused);
	generic_.FinishRestoreSignal.Connect(this, &PauseFeature::FinishRestore);

	if (ORIG_SpawnPlayer)
	{
		int ptnNumber = GetPatternIndex((void**)&ORIG_SpawnPlayer);

		switch (ptnNumber)
		{
		case 0:
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 5));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 18));
			break;

		case 1:
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 8));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 21));
			break;

		case 2: // 4104 is the same as 5135 here.
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 5));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 18));
			break;

		case 3: // 2257546 is the same as 5339 here.
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 8));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 21));
			break;

		case 4:
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 26));
			//pGameServer = (*(void **)(pSpawnPlayer + 21)); - We get this one from SV_ActivateServer in OE.
			break;

		case 5: // 6879 is the same as 5339 here.
			pM_bLoadgame = (*(bool**)(ORIG_SpawnPlayer + 8));
			pGameServer = (*(void**)(ORIG_SpawnPlayer + 21));
			break;

		default:
			Warning("Spawnplayer did not have a matching switch-case statement!\n");
			break;
		}

		DevMsg("m_bLoadGame is situated at %p.\n", pM_bLoadgame);

#if !defined(OE)
		DevMsg("pGameServer is %p.\n", pGameServer);
#endif
	}

	// SV_ActivateServer
	if (generic_.ORIG_SV_ActivateServer)
	{
		int ptnNumber = GetPatternIndex((void**)&generic_.ORIG_SV_ActivateServer);
		switch (ptnNumber)
		{
		case 3:
			pGameServer = (*(void**)((int)generic_.ORIG_SV_ActivateServer + 223));
			break;
		}

#if defined(OE)
		DevMsg("pGameServer is %p.\n", pGameServer);
#endif
	}

	if (!ORIG_SpawnPlayer || !generic_.ORIG_SV_ActivateServer)
	{
		Warning("y_spt_pause 2 has no effect.\n");
	}

	// FinishRestore
	if (!generic_.ORIG_FinishRestore)
	{
		Warning("y_spt_pause 1 has no effect.\n");
	}

	// SetPaused
	if (!generic_.ORIG_SetPaused)
	{
		Warning("y_spt_pause has no effect.\n");
	}

}

void PauseFeature::UnloadFeature() {}

void PauseFeature::SV_ActivateServer(bool result)
{
	DevMsg("Engine call: SV_ActivateServer() => %s;\n", (result ? "true" : "false"));

	if (generic_.ORIG_SetPaused && pM_bLoadgame && pGameServer)
	{
		if ((y_spt_pause.GetInt() == 2) && *pM_bLoadgame)
		{
			generic_.ORIG_SetPaused((void*)pGameServer, 0, true);
			DevMsg("Pausing...\n");

			generic_.shouldPreventNextUnpause = true;
		}
	}
}

void PauseFeature::FinishRestore(void* thisptr, int edx)
{
	DevMsg("Engine call: FinishRestore();\n");

	if (generic_.ORIG_SetPaused && (y_spt_pause.GetInt() == 1))
	{
		generic_.ORIG_SetPaused(thisptr, 0, true);
		DevMsg("Pausing...\n");

		generic_.shouldPreventNextUnpause = true;
	}
}

void PauseFeature::SetPaused(void* thisptr, int edx, bool paused)
{
	if (pM_bLoadgame)
	{
		DevMsg("Engine call: SetPaused( %s ); m_bLoadgame = %s\n",
			(paused ? "true" : "false"),
			(*pM_bLoadgame ? "true" : "false"));
	}
	else
	{
		DevMsg("Engine call: SetPaused( %s );\n", (paused ? "true" : "false"));
	}
}
