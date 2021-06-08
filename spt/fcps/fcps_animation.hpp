#pragma once

#include "fcps_memory_repr.hpp"

// clang-format off

namespace fcps {

	class FcpsAnimator {
	private:
		bool isAnimating, isSetToStep;
		int totalSteps;
		float stepDuration;
	public:
		FcpsAnimator();
		void stopAnimation();
		void beginAnimation(int from, int to, double seconds);
		void stepAnimation();
	};


	extern FcpsAnimator fcpsAnimator;


	inline void stopFcpsAnimation() {
		fcpsAnimator.stopAnimation();
	}
}
