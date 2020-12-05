#pragma once

#include <vector>
#include "mathlib\vector.h"

namespace ljstats
{
	enum class AccelDirection {
		Left, Forward, Right
	};

	struct SegmentStats {
		void Reset();
		float Sync();

		int ticks;
		int accelTicks;
		int gainTicks;
		AccelDirection dir;
		float totalAccel;
		float positiveAccel;
		float negativeAccel;
		float distanceCovered;
		float startVel;
	};

	struct JumpStats {
		int StrafeCount();
		int TotalTicks();
		float Sync();
		float AccelPerTick();
		float TotalAccel();
		float NegativeAccel();
		float PositiveAccel();
		float TimeAccelerating();
		float Length();
		float Prestrafe();
		float Straightness();

		void Reset();

		Vector jumpSpot;
		Vector endSpot;
		float startVel;
		std::vector<SegmentStats> segments;
	};

	void OnJump();
	void OnTick();
	extern JumpStats lastJump;
} // namespace ljstats