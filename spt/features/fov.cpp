#include "stdafx.h"
#include "..\feature.hpp"
#include "..\OrangeBox\cvars.hpp"

typedef void(__fastcall* _CViewRender__OnRenderStart)(void* thisptr, int edx);

ConVar _y_spt_force_fov("_y_spt_force_fov", "0", 0, "Force FOV to some value.");

// FOV related features
class FOVFeatures : public Feature
{
public:
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

private:
	_CViewRender__OnRenderStart ORIG_CViewRender__OnRenderStart;
	static void __fastcall HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx);
};

static FOVFeatures _fov;

bool FOVFeatures::ShouldLoadFeature()
{
// Atm only works in OE
#ifdef OE
	return true;
#else
	return false;
#endif
}

void FOVFeatures::InitHooks()
{
	HOOK_FUNCTION(client, CViewRender__OnRenderStart);
}

void FOVFeatures::LoadFeature() {}

void FOVFeatures::UnloadFeature() {}

void __fastcall FOVFeatures::HOOKED_CViewRender__OnRenderStart(void* thisptr, int edx)
{
	_fov.ORIG_CViewRender__OnRenderStart(thisptr, edx);

	if (!_viewmodel_fov || !_y_spt_force_fov.GetBool())
		return;

	float* fov = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisptr) + 52);
	float* fovViewmodel = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(thisptr) + 56);
	*fov = _y_spt_force_fov.GetFloat();
	*fovViewmodel = _viewmodel_fov->GetFloat();
}
