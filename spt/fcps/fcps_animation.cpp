#include "stdafx.h"

#include "fcps_animation.hpp"
#include "fcps_override.hpp"
#include "..\OrangeBox\spt-serverplugin.hpp"

// clang-format off

namespace fcps {

	// it shouldn't really matter what this value is, but it should be used for manual step stuff for consistency
	#define MANUAL_STEP_DURATION 1.0


	FcpsAnimator fcpsAnimator = FcpsAnimator(); // global animator object


	FcpsAnimator::FcpsAnimator() : isAnimating(false) {}


	FcpsAnimator::~FcpsAnimator() {}


	void FcpsAnimator::stopAnimation() {
		isAnimating = false;
		// TODO also other stuff
	}


	void FcpsAnimator::beginAnimation(int from, int to, double seconds, FixedFcpsQueue* fcpsQueue) {
		isSetToManualStep = seconds == 0;
		curSubStepTime = isSetToManualStep ? MANUAL_STEP_DURATION : 0; // in manual step I always want to be at the very end of the current sub step
		curQueue = fcpsQueue;
		fromId = curId = from;
		toId = to;
		curQueue = fcpsQueue;
		calcSubStepDurations(seconds);
		isAnimating = true;
		curStep = AS_DrawBBox;
		lastDrawTime = hacks::curTime();

		// animation vars
		curLoop = 0;
		cornerIdx = 0;
		curValidationRay = 0;
		FcpsEvent* nextEvent = curQueue->getEventWithId(from);
		curCenter = nextEvent->zNudgedCenter;
		curMins = nextEvent->adjustedMins;
		curMaxs = nextEvent->adjustedMaxs;
		// TODO, don't forget to check map name
	}


	/*
	* Derivations of the mafs:
	* Suppose you have an animation with 3 steps A,B,C. Each step has a total of a,b,c substeps respectively (these can be calculated).
	* We have the relative lengths of each substep x,y,z. These are given in the relativeSubstepTimes array.
	* We want to figure out the value R - how much to scale x,y,z by for the total animation length to be W.
	* 
	* We then have ai+bj+ck=W where i=Rx, j=Ry, and k=Rz (i,j,k are rescaled x,y,z by the factor R).
	* => R(ax+by+cz)=W
	* => R=W/(ax+by+cz)
	*/
	void FcpsAnimator::calcSubStepDurations(double seconds) {
		if (isSetToManualStep) {
			// set to constant speed
			for (int i = 0; i < AS_Count; i++)
				subStepDurations[i] = MANUAL_STEP_DURATION;
			return;
		}
		// figure out how many substeps each step is gonna have
		int subStepCounts[AS_Count] = {};
		for (int i = 0; i <= toId - fromId; i++) {

			FcpsEvent* fe = curQueue->getEventWithId(fromId + i);

			subStepCounts[AS_DrawBBox]++;
			subStepCounts[AS_TestTrace] += fe->loopStartCount;
			subStepCounts[AS_CornerValidation] += fe->loopFinishCount * 8;
			
			for (int loop = 0; loop < fe->loopFinishCount; loop++) {
				int inboundsCorners = 0;
				for (int c = 0; c < 8; c++)
					inboundsCorners += !fe->loops[loop].cornersOob[c];
				subStepCounts[AS_CornerRays] += inboundsCorners * 7; // a ray from each inbounds corner to every other corner
			}

			subStepCounts[AS_NudgeAndAdjust] += fe->loopFinishCount;
			subStepCounts[AS_Success] += fe->wasSuccess;
			subStepCounts[AS_Revert] += !fe->wasSuccess;
		}
		// figure out the ax+by+cz sum
		double unscaledTime = 0;
		for (int i = 0; i < AS_Count; i++)
			if (shouldDrawStep[i])
				unscaledTime += subStepCounts[i] * relativeSubstepTimes[i];
		// get the i,j,k values and store them
		for (int i = 0; i < AS_Count; i++)
			subStepDurations[i] = seconds / unscaledTime * relativeSubstepTimes[i];
	}


	void FcpsAnimator::stepAnimation() {
		if (!isAnimating) {
			Msg("No animation in progress!\n");
			return;
		}
		if (!isSetToManualStep) {
			Msg("Current animation is not in step mode\n");
			return;
		}
		curSubStepTime += MANUAL_STEP_DURATION;
	}


	void stopFcpsAnimation() {
		fcpsAnimator.stopAnimation();
	}


	void FcpsAnimator::draw() {
		if (!isAnimating || curStep == AS_Finished)
			return;
		auto vdo = GetDebugOverlay();
		if (!vdo)
			return;
		float dur = NDEBUG_PERSIST_TILL_NEXT_SERVER;
		// TODO there's gonna be some problems with alpha stacking or whatever
		if (!isSetToManualStep) {
			float curTime = hacks::curTime();
			curSubStepTime += curTime - lastDrawTime;
			lastDrawTime = curTime;
		}
		//double subStepFrac = std::fmin(1, curSubStepTime / subStepDurations[curStep]); // how far are we into the current substep, 0..1 range
		//Assert(subStepFrac >= 0);
		bool hasDrawnBBoxThisFrame = false;
		for (;;) {
			Assert(curQueue);
			FcpsEvent* fe = curQueue->getEventWithId(curId);
			Assert(fe);
			// draw current sub step
			if (shouldDrawStep[curStep]) {
				if (!hasDrawnBBoxThisFrame && curStep != AS_Revert && curStep != AS_Success) {
					vdo->AddBoxOverlay(curCenter, fe->entMins, fe->entMaxs, vec3_angle, 255, 0, 0, 25, dur); // original bounds
					vdo->AddBoxOverlay(curCenter, curMins, curMaxs, vec3_angle, 255, 255, 0, 50, dur); // extended bounds
					hasDrawnBBoxThisFrame = true;
				}
				switch (curStep) {
					case AS_CornerValidation:
						for (int i = 0; i <= cornerIdx; i++)
							vdo->AddTextOverlay(fe->loops[curLoop].corners[i], dur, "%d: %s", i + 1, fe->loops[curLoop].cornersOob[i] ? "OOB" : "INBOUNDS");
						break;
					case AS_CornerRays: {
						auto& curValidationTest = fe->loops[curLoop].validationRayChecks[curValidationRay];
						for (int i = 0; i < 2; i++) {
							if (curValidationTest.trace[i].startsolid)
								continue;
							Vector start = curValidationTest.trace[i].startpos;
							Vector end = curValidationTest.trace[i].endpos;
							Vector hitPos = lerp(start, end, curValidationTest.trace[i].fraction);
							vdo->AddLineOverlay(start, hitPos, 0, 255, 0, 0, dur);
							vdo->AddLineOverlay(hitPos, end, 255, 0, 0, 0, dur);
						}
						break;
					}
					case AS_Revert:
						vdo->AddBoxOverlay(fe->zNudgedCenter, fe->entMins, fe->entMaxs, vec3_angle, 255, 0, 0, 50, dur);
						break;
					case AS_Success:
						vdo->AddBoxOverlay(fe->newPos, fe->entMins, fe->entMaxs, vec3_angle, 0, 255, 0, 50, dur);
						break;
				}
			}

			// go to next substep if it's time
			if (curSubStepTime - subStepDurations[curStep] > 0 || !shouldDrawStep[curStep]) { // this is > instead of >= to make the sure manual step doesn't draw 2 steps on the first draw
				if (shouldDrawStep[curStep])
					curSubStepTime -= subStepDurations[curStep];
				// Update any animations vars for the next draw. The draw will assume any relevant vars are valid (e.g. corner2 is the index of a valid inbounds corner).
				// We must skip over substeps that won't be drawn here, because the if check ^ means that we assume the first half of the loop is guaranteed to draw
				// something and "take time" to do so. If the current substep is done, advance to the next one. (this is poorly worded, too bad)
				switch (curStep) {
					case AS_DrawBBox:
						curStep = AS_TestTrace;
						curLoop = 0;
						break;
					case AS_TestTrace:
						// after the trace, FCPS either succeeds or goes on to the rest of the loop
						curStep = curLoop == fe->loopStartCount - 1 ? AS_Success : AS_CornerValidation;
						cornerIdx = 0;
						break;
					case AS_CornerValidation:
						if (++cornerIdx >= 8) {
							// After checking all corners for validity, FCPS fires rays. Check if there's any rays to fire.
							for (curValidationRay = 0; curValidationRay < fe->loops[curLoop].validationRayCheckCount; curValidationRay++) {
								auto& rayCheck = fe->loops[curLoop].validationRayChecks[curValidationRay];
								if (!rayCheck.trace[0].startsolid || !rayCheck.trace[1].startsolid)
									break;
							}
							if (curValidationRay == fe->loops[curLoop].validationRayCheckCount)
								curStep = AS_NudgeAndAdjust;
							else
								curStep = AS_CornerRays;
						}
						break;
					case AS_CornerRays:
						for (;;) {
							if (++curValidationRay == fe->loops[curLoop].validationRayCheckCount) {
								curStep = AS_NudgeAndAdjust;
								break;
							} else {
								auto& rayCheck = fe->loops[curLoop].validationRayChecks[curValidationRay];
								if (!rayCheck.trace[0].startsolid || !rayCheck.trace[1].startsolid)
									break;
							}
						}
						break;
					case AS_NudgeAndAdjust:
						if (++curLoop == 100) {
							curStep = AS_Revert;
						} else {
							curCenter = fe->loops[curLoop].newCenter;
							curMins = fe->loops[curLoop].newMins;
							curMaxs = fe->loops[curLoop].newMaxs;
							curStep = AS_TestTrace;
						}
						break;
					case AS_Success:
					case AS_Revert:
						if (++curId > toId) {
							curStep = AS_Finished;
							return;
						} else {
							curStep = AS_DrawBBox;
							FcpsEvent* nextEvent = curQueue->getEventWithId(curId);
							curCenter = nextEvent->zNudgedCenter;
							curMins = nextEvent->adjustedMins;
							curMaxs = nextEvent->adjustedMaxs;
						}
						break;
				}
			} else {
				break;
			}
		}
	}


	CON_COMMAND(un_stop_fcps_animation, "Stops any in-progress FCPS animation.") {
		stopFcpsAnimation();
	}


	CON_COMMAND(un_animate_fcps_events, "[x]|[x:y] [seconds] - animates the FCPS events with the given ID or range of IDs over the given number of seconds, use 0s to use un_fcps_animation_step.") {
		if (args.ArgC() < 3) {
			Msg("you must specify event IDs and a number of seconds to animate for\n");
			return;
		}
		unsigned long lower, upper;
		if (!parseFcpsEventRange(args.Arg(1), lower, upper, RecordedFcpsQueue)) {
			Msg("\"%s\" is not a valid value or a valid range of values (check if events with the given values are loaded)\n", args.Arg(1));
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
		fcpsAnimator.beginAnimation(lower, upper, seconds, RecordedFcpsQueue); // TODO change this to LoadedQueue eventually
	}


	CON_COMMAND(un_fcps_animation_step, "Sets the current FCPS animation to the next step.") {
		fcpsAnimator.stepAnimation();
	}
}