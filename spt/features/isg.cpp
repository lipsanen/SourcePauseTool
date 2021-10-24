#include "stdafx.h"
#include "..\feature.hpp"
#include "..\OrangeBox\patterns.hpp"
#include "..\utils\game_detection.hpp"

#include <functional>

#include "convar.h"

#if defined(SSDK2007) || defined(SSDK2013)

class ISGFeature : public Feature
{
public:
	bool* isgFlagPtr;

protected:
	uint32_t ORIG_MiddleOfRecheck_ov_element;

	virtual bool ShouldLoadFeature() override
	{
		return utils::DoesGameLookLikePortal();
	}

	virtual void InitHooks() override
	{
		auto callback = PATTERN_CALLBACK {
			if (ORIG_MiddleOfRecheck_ov_element)
				this->isgFlagPtr = *(bool**)(ORIG_MiddleOfRecheck_ov_element + 2);
			else
				Warning("y_spt_hud_isg 1 and y_spt_set_isg have no effect\n");
		};
		FIND_PATTERN_WITH_CALLBACK(vphysics, MiddleOfRecheck_ov_element, ORIG_MiddleOfRecheck_ov_element, callback);
	}

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override
	{
	}
};

static ISGFeature _isg;
ConCommand* y_spt_set_isg;


void CC_Set_ISG(const CCommand& args)
{
	if (_isg.isgFlagPtr)
		*_isg.isgFlagPtr = args.ArgC() == 1 || atoi(args[1]);
	else
		Warning("y_spt_set_isg has no effect\n");
}

void ISGFeature::LoadFeature()
{
	y_spt_set_isg = new ConCommand("y_spt_set_isg",
		CC_Set_ISG,
		"Sets the state of ISG in the game (1 or 0), no arguments means 1",
		FCVAR_DONTRECORD | FCVAR_CHEAT);
}

bool IsISGActive()
{
	if(_isg.isgFlagPtr)
		return *_isg.isgFlagPtr;
	else
		return false;
}
#else

bool IsISGActive()
{
	return false;
}

#endif