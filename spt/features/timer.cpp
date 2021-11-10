#include "stdafx.h"
#include "..\feature.hpp"
#include "generic.hpp"

#include "convar.h"

// Feature description
class Timer : public Feature
{
public:
	void StartTimer()
	{
		timerRunning = true;
	}
	void StopTimer()
	{
		timerRunning = false;
	}
	void ResetTimer()
	{
		ticksPassed = 0;
		timerRunning = false;
	}
	unsigned int GetTicksPassed() const
	{
		return ticksPassed;
	}
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;
private:
	void Tick();
	int ticksPassed;
	bool timerRunning;
};

static Timer _timer;

bool Timer::ShouldLoadFeature()
{
	return true;
}

void Timer::InitHooks() {
}

void Timer::LoadFeature() 
{
	ticksPassed = 0;
	timerRunning = false;
	generic_.TickSignal.Connect(this, &Timer::Tick);
}

void Timer::UnloadFeature() {}

void Timer::Tick()
{
	if(timerRunning)
		++ticksPassed;
}


CON_COMMAND(y_spt_timer_start, "Starts the SPT timer.")
{
	_timer.StartTimer();
}

CON_COMMAND(y_spt_timer_stop, "Stops the SPT timer and prints the current time.")
{
	_timer.StopTimer();
	Warning("Current time (in ticks): %u\n", _timer.GetTicksPassed());
}

CON_COMMAND(y_spt_timer_reset, "Stops and resets the SPT timer.")
{
	_timer.ResetTimer();
}

CON_COMMAND(y_spt_timer_print, "Prints the current time of the SPT timer.")
{
	Warning("Current time (in ticks): %u\n", _timer.GetTicksPassed());
}