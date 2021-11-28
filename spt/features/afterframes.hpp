#pragma once

#include "..\feature.hpp"
#include "thirdparty\Signal.h"

struct afterframes_entry_t
{
	afterframes_entry_t(long long int framesLeft, std::string command)
	    : framesLeft(framesLeft), command(std::move(command))
	{
	}
	afterframes_entry_t() {}
	long long int framesLeft;
	std::string command;
};

// This feature enables _y_spt_afterframes
class AfterframesFeature : public Feature
{
public:
	void AddAfterFramesEntry(afterframes_entry_t entry);

	void DelayAfterframesQueue(int delay);
	void ResetAfterframesQueue();
	void PauseAfterframesQueue();
	void ResumeAfterframesQueue();

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void LoadFeature() override;
	virtual void UnloadFeature() override;

private:
	void OnFrame();
	void SV_ActivateServer(bool result);
	void FinishRestore(void* thisptr, int edx);
	void SetPaused(void* thisptr, int edx, bool paused);
	std::vector<afterframes_entry_t> afterframesQueue;
	bool afterframesPaused;
	int afterframesDelay;
};

extern AfterframesFeature spt_afterframes;