#include "stdafx.h"

#include "fcps_animation.hpp"

// clang-format off

namespace fcps {

	FcpsAnimator fcpsAnimator = FcpsAnimator(); // global animator object


	FcpsAnimator::FcpsAnimator() : isAnimating(false) {}


	void FcpsAnimator::stopAnimation() {
		isAnimating = false;
		// TODO also other stuff
	}


	void FcpsAnimator::beginAnimation(int from, int to, double seconds) {
		isSetToStep = seconds == 0;
		// TODO, don't forget to check map name
	}


	void FcpsAnimator::stepAnimation() {
		if (!isAnimating) {
			Msg("No animation in progress!\n");
			return;
		}
		if (!isSetToStep) {
			Msg("Current animation is not in step mode\n");
			return;
		}
		// TODO
	}


	CON_COMMAND(un_stop_fcps_animation, "Stops any in-progress FCPS animations.") {
		stopFcpsAnimation();
	}


	CON_COMMAND(un_animate_fcps_events, "[x]|[x:y] [seconds] - animates the FCPS events with the given ID or range of IDs over the given number of seconds, use 0s to use un_fcps_animation_step.") {
		if (args.ArgC() < 3) {
			Msg("you must specify event IDs and a number of seconds to animate for\n");
			return;
		}
		unsigned long lower, upper;
		if (!parseFcpsEventRange(args.Arg(1), lower, upper, LoadedFcpsQueue)) {
			Msg("\"%s\" is not a valid value or a valid range of values (check if events with the given values are loaded)\n", args.Arg(2));
			return;
		}
		char* endPtr;
		double seconds = std::strtod(args.Arg(2), &endPtr);
		while (*endPtr) {
			if (seconds < 0 || !isspace(*endPtr++)) {
				Msg("\"%s\" is not a valid number of seconds\n", args.Arg(2));
				return;
			}
		}
		fcpsAnimator.beginAnimation(lower, upper, seconds);
	}


	CON_COMMAND(un_fcps_animation_step, "Sets the current FCPS animation to the next step.") {
		fcpsAnimator.stepAnimation();
	}
}