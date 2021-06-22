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
		lastDrawFrame = hacks::frameCount();

		// substep specific vars
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
			subStepDurations[i] = shouldDrawStep[i] ? (seconds / unscaledTime * relativeSubstepTimes[i]) : 0;
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


	void FcpsAnimator::drawRayTest(float duration) {

		auto vdo = GetDebugOverlay();
		auto& loopInfo = curQueue->getEventWithId(curId)->loops[curLoop];
		auto& rayCheck = loopInfo.validationRayChecks[curValidationRay];

		Assert(!rayCheck.trace[0].startsolid || !rayCheck.trace[1].startsolid); // at least one ray should have been fired
		Vector corner[2];
		Vector impact[2];
		bool exceededThresholdFromStart[2];
		bool exceededThresholdFromEnd[2];
		const float distThresholdSqr = 0.01;
		const Vector impactMaxs = Vector(1, 1, 1);
		const Vector raySuccessMaxs = impactMaxs / 1.8;
		const Vector rayFailMaxs = raySuccessMaxs / 1.5;

		for (int i = 0; i < 2; i++) {
			corner[i] = loopInfo.corners[rayCheck.cornerIdx[i]];
			impact[i] = rayCheck.trace[i].startsolid ? corner[i] : rayCheck.trace[i].endpos;
		}

		// check if the ray travelled at least a tiny bit (and didn't get all the way to the other corner)
		for (int i = 0; i < 2; i++) {
			exceededThresholdFromStart[i] = (corner[i] - impact[i]).LengthSqr() > distThresholdSqr;
			exceededThresholdFromEnd[i] = (corner[(i+1)%2] - impact[i]).LengthSqr() > distThresholdSqr;
		}
		// part of the trace that passed through "passable" space
		Vector testRayExtents;
		for (int i = 0; i < 2; i++) {
			if (exceededThresholdFromStart[i]) {
				vdo->AddSweptBoxOverlay(corner[i], impact[i], -raySuccessMaxs, raySuccessMaxs, vec3_angle, 0, 255, 0, 100, duration);
				testRayExtents = rayCheck.ray[i].m_Extents*1.001; // prevent z-fighting
			}
		}
		// draw the ray with proper extents
		if (exceededThresholdFromStart[0] || exceededThresholdFromStart[1])
			vdo->AddSweptBoxOverlay(corner[0], corner[1], -testRayExtents, testRayExtents, vec3_angle, 255, 255, 255, 100, duration);
		// part of the trace that passed through "non-passable" space
		if (exceededThresholdFromEnd[0] || exceededThresholdFromEnd[1]) {
			vdo->AddSweptBoxOverlay(impact[0], impact[1], -rayFailMaxs, rayFailMaxs, vec3_angle, 255, 0, 0, 100, duration);
			// draw box at impact location
			for (int i = 0; i < 2; i++)
				if (exceededThresholdFromStart[i])
					vdo->AddBoxOverlay(impact[i], -impactMaxs, impactMaxs, vec3_angle, 0, 255, 0, 75, duration);
		}
	}


	void FcpsAnimator::draw() {
		if (!isAnimating || curStep == AS_Finished)
			return;
		if (hacks::frameCount() == lastDrawFrame)
			return;
		lastDrawFrame = hacks::frameCount();
		auto vdo = GetDebugOverlay();
		if (!vdo)
			return;
		float dur = NDEBUG_PERSIST_TILL_NEXT_SERVER;
		if (!isSetToManualStep) {
			// TODO now that I'm drawing once per frame this gets updated by like 0.1s every draw() call, not sure why
			curSubStepTime += hacks::curTime() - lastDrawTime;
			lastDrawTime = hacks::curTime();
		}
		//double subStepFrac = std::fmin(1, curSubStepTime / subStepDurations[curStep]); // how far are we into the current substep, 0..1 range
		//Assert(subStepFrac >= 0);

		// If we have a fast enough animation speed it's possible to draw several substeps on the same frame.
		// This is a problem since the bbox of the ents get drawn every time (and has an alpha component), so just keep track if we've done that already.
		bool hasDrawnBBoxThisFrame = false;
		bool hasDrawnPlayerBBoxThisFrame = false;
		for (;;) {
			Assert(curQueue);
			FcpsEvent* fe = curQueue->getEventWithId(curId);
			Assert(fe);
			// draw current substep
			if (shouldDrawStep[curStep]) {
				if (!hasDrawnBBoxThisFrame && curStep != AS_Revert && curStep != AS_Success) {
					vdo->AddBoxOverlay(curCenter, fe->entMins, fe->entMaxs, vec3_angle, 255, 0, 0, 25, dur); // original bounds
					vdo->AddBoxOverlay(curCenter, curMins, curMaxs, vec3_angle, 255, 255, 0, 25, dur); // extended bounds
					hasDrawnBBoxThisFrame = true;
				}
				if (!hasDrawnPlayerBBoxThisFrame && !fe->wasRunOnPlayer) {
					vdo->AddBoxOverlay(vec3_origin, fe->playerMins, fe->playerMaxs, vec3_angle, 255, 0, 255, 25, dur);
					hasDrawnPlayerBBoxThisFrame = true;
				}
				switch (curStep) {
					case AS_CornerValidation:
						for (int i = 0; i <= cornerIdx; i++)
							vdo->AddTextOverlay(fe->loops[curLoop].corners[i], dur, "%d: %s", i + 1, fe->loops[curLoop].cornersOob[i] ? "OOB" : "INBOUNDS");
						break;
					case AS_CornerRays:
						drawRayTest(dur);
						break;
					case AS_Revert:
						vdo->AddBoxOverlay(fe->originalCenter, fe->entMins, fe->entMaxs, vec3_angle, 255, 0, 0, 50, dur);
						break;
					case AS_Success:
						vdo->AddBoxOverlay(curCenter, fe->entMins, fe->entMaxs, vec3_angle, 0, 255, 0, 50, dur);
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
						if (fe->wasSuccess && curLoop == fe->loopStartCount - 1)
							curStep = AS_Success;
						else
							curStep = AS_CornerValidation;
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
							Vector oldCenter = curCenter;
							curCenter = fe->loops[curLoop - 1].newCenter;
							curMins = fe->loops[curLoop - 1].newMins;
							curMaxs = fe->loops[curLoop - 1].newMaxs;
							curStep = AS_TestTrace;
							if (curCenter.DistToSqr(oldCenter) > 10)
								hasDrawnBBoxThisFrame = false; // allow redrawning of bbox if it won't be exactly in the same spot
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
							hasDrawnBBoxThisFrame = false;
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