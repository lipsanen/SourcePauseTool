#pragma once

#include "fcps_memory_repr.hpp"

// clang-format off

namespace fcps {

	class FcpsAnimator {
	private:
		bool isAnimating;
	public:
		FcpsAnimator();
		void stopAnimation();
	};


	extern FcpsAnimator fcpsAnimator;


	inline void stopFcpsAnimation() {
		fcpsAnimator.stopAnimation();
	}
}
