#include "stdafx.h"

#include "fcps_animation.hpp"

// clang-format off

namespace fcps {

	FcpsAnimator fcpsAnimator = FcpsAnimator(); // global animator object


	FcpsAnimator::FcpsAnimator() : isAnimating(false) {}


	void FcpsAnimator::stopAnimation() {
		isAnimating = false;
	}
}