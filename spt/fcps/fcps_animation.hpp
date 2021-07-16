#pragma once

#include "fcps_memory_repr.hpp"
#include "convar.h"

// clang-format off

namespace fcps {

	class FcpsAnimator {
	private:

		/*
		* The overal animation is going to be done on each event and will be broken up into steps,
		* with each step consisting of possibly multiple simple substeps (e.g. animate a ray) or a single complex step.
		*/
		enum AnimationStep {
			AS_DrawBBox,
			AS_TestTrace,        // tests if the space is empty and fires a ray towards original location
			AS_CornerValidation, // tests which corners are oob
			AS_CornerRays,       // fires rays between the corners
			AS_NudgeAndAdjust,   // nudges the entity and adjusts the mins/maxs
			AS_Success,          // in case of FCPS success the entity position is changed
			AS_Revert,           // in case of FCPS failure the entity position is reverted
			AS_Finished,
			AS_Count = AS_Finished
		};

		// general info
		bool isAnimating, isSetToManualStep;
		double curSubStepTime; // how many seconds are we into the current substep?
		AnimationStep curStep;
		int fromId, curId, toId; // event IDs
		FixedFcpsQueue* curQueue;
		double lastDrawTime;
		int lastDrawFrame, lastDrawTick;
		bool stepButtonHeld;
		bool skipUntilNextStep;
		float secondsSinceLastHeldStep; // how many seconds since we last incremented the substep (while holding +fcps_step_animation)
		const float secondsPerHeldStep = 0.15f; // won't work properly if the value is too small (e.g. 3 ticks)

		// For any given step, each substep is gonna take the same amount of time e.g. all rays fired during the CornerRays step will all take the same amount of time.
		double subStepDurations[AS_Count];
		// the relative lengths of the substeps
		double relativeSubstepTimes[AS_Count] = {5, 1.5f, 1, 1, 1, 5, 5};
		// which steps do we draw?
		bool shouldDrawStep[AS_Count] = {1, 1, 1, 1, 1, 1, 1};

		// substep animations vars
		int curLoopIdx;
		int cornerIdx;
		int curTwcIdx; // which twc are we animating?
		int curTwcCount; // how many have we animated so far? (only used for hud progress)
		Vector curCenter;
		bool nextSubStepIsTrace;

		void calcSubStepDurations(double seconds);
		void drawRaysFromCorners(float duration);
		bool canManualStep();

	public:
		FcpsAnimator();
		~FcpsAnimator();
		void stopAnimation();
		void beginAnimation(int from, int to, double seconds, FixedFcpsQueue* fcpsQueue);
		void stepAnimation();
		void skipStepType();
		void draw();
		void adjustAnimationSpeed(double seconds);
		void setHeldStepButton(bool pressed);
		std::wstring getProgressString();
	};


	extern FcpsAnimator fcpsAnimator;

	extern ConVar fcps_animation_time;

	extern ConVar fcps_hud_progress;


	inline void drawAnimationFrame() {
		fcpsAnimator.draw();
	}
}
