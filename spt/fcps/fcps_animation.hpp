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

		/*template<typename T>
		T easeInterp(T t1, T t2, float frac); // x^2 * (3 - 2x)*/

		// general info
		bool isAnimating, isSetToManualStep;
		double curSubStepTime; // how many seconds are we into the current substep?
		AnimationStep curStep;
		int fromId, curId, toId; // event IDs
		FixedFcpsQueue* curQueue;
		double lastDrawTime;
		int lastDrawFrame;

		// For any given step, each substep is gonna take the same amount of time e.g. all rays fired during the CornerRays step will all take the same amount of time.
		double subStepDurations[AS_Count];
		// the relative lengths of the substeps
		double relativeSubstepTimes[AS_Count] = {5, 1, 1, 1, 1, 5, 5};
		// which steps do we draw?
		bool shouldDrawStep[AS_Count] = {1, 1, 1, 1, 1, 1, 1};

		// substep animations vars
		int curLoopIdx;
		int cornerIdx;
		int curValidationRay;
		Vector curCenter, curMins, curMaxs;

		void calcSubStepDurations(double seconds);
		void drawRayTest(float duration);
		void loadAnimationSettings();


	public:
		FcpsAnimator();
		~FcpsAnimator();
		void stopAnimation();
		void beginAnimation(int from, int to, double seconds, FixedFcpsQueue* fcpsQueue);
		void stepAnimation();
		void draw();
		void adjustAnimationSpeed(double seconds);
	};


	extern FcpsAnimator fcpsAnimator;

	extern ConVar un_fcps_animation_speed;


	inline void drawAnimationFrame() {
		fcpsAnimator.draw();
	}
}
