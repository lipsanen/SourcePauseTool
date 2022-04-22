#pragma once

#include "..\feature.hpp"
#include "convar.hpp"

// y_spt_pause stuff
class PauseFeature : public FeatureWrapper<PauseFeature>
{
public:
	bool InLoad();
protected:
	virtual void InitHooks() override;
	virtual void LoadFeature() override;

private:
	uintptr_t ORIG_SpawnPlayer = 0;
	bool* pM_bLoadgame = nullptr;
	void* pGameServer = nullptr;

	void SV_ActivateServer(bool result);
	void FinishRestore(void* thisptr, int edx);
	void SetPaused(void* thisptr, int edx, bool paused);
};

extern PauseFeature spt_pause;