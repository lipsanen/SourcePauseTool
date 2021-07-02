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
		curLoopIdx = 0;
		cornerIdx = 0;
		curValidationRayIdx = 0;
		FcpsEvent* nextEvent = curQueue->getEventWithId(from);
		curCenter = nextEvent->origCenter;
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
			subStepCounts[AS_TestTrace] += fe->loopStartCount - 1; // first trace does nothing so -1
			subStepCounts[AS_CornerValidation] += fe->loopFinishCount * 8 + (i ? 1 : 0); // +1 for one substep without any info (except for the first time)
			
			for (int loop = 0; loop < fe->loopFinishCount; loop++) {
				int inboundsCorners = 0;
				for (int c = 0; c < 8; c++)
					inboundsCorners += !fe->loops[loop].cornersOob[c];
				// a ray from each inbounds corner to every other corner
				subStepCounts[AS_CornerRays] += inboundsCorners * 7 + 1; // +1 for one substep to show all validations
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


	void FcpsAnimator::adjustAnimationSpeed(double seconds) {
		isSetToManualStep = seconds == 0;
		double curSubStepFrac = (isAnimating && curStep != AS_Finished) ? curSubStepTime / subStepDurations[curStep] : 0;
		calcSubStepDurations(seconds);
		if (isSetToManualStep)
			curSubStepTime = MANUAL_STEP_DURATION;
		else
			curSubStepTime = (isAnimating && curStep != AS_Finished) ? curSubStepFrac * subStepDurations[curStep] : 0;
	}


	void FcpsAnimator::stepAnimation() {
		if (!isAnimating)
			return;
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
		auto& loopInfo = curQueue->getEventWithId(curId)->loops[curLoopIdx];

		// last substep - don't draw rays, just draw the total validation values
		if (curValidationRayIdx == -1) {
			for (int i = 0; i < 8; i++) {
				if (loopInfo.cornerValidation[i] <= 0)
					vdo->AddTextOverlay(loopInfo.corners[i], duration, "%d: 0.00", i); // if the validation is less than 0 it's ignored
				else
					vdo->AddTextOverlay(loopInfo.corners[i], duration, "%d: %.2f", i, loopInfo.cornerValidation[i]);
			}
			return;
		}

		auto& rayCheck = loopInfo.validationRayChecks[curValidationRayIdx];

		Assert(!rayCheck.trace[0].startsolid || !rayCheck.trace[1].startsolid); // at least one ray should have been fired
		Vector corner[2];
		Vector impact[2];
		bool exceededThresholdFromStart[2];
		bool exceededThresholdFromEnd[2];
		const float distThresholdSqr = 0.01;
		const Vector impactMaxs = Vector(1, 1, 1);
		const Vector raySuccessMaxs = impactMaxs / 1.8;
		const Vector rayFailMaxs = raySuccessMaxs / 1.5;
		const bool drawProperExtentsWithImpact = true;

		for (int i = 0; i < 2; i++) {
			corner[i] = loopInfo.corners[rayCheck.cornerIdx[i]];
			impact[i] = rayCheck.trace[i].startsolid ? corner[i] : rayCheck.trace[i].endpos;
		}

		// check if the ray travelled at least a tiny bit (and didn't get all the way to the other corner)
		for (int i = 0; i < 2; i++) {
			exceededThresholdFromStart[i] = (corner[i] - impact[i]).LengthSqr() > distThresholdSqr;
			exceededThresholdFromEnd[i] = (corner[(i+1)%2] - impact[i]).LengthSqr() > distThresholdSqr;
		}
		Vector testRayExtents;
		for (int i = 0; i < 2; i++) {
			// save the extends of whichever ray was traced
			if (!rayCheck.trace[i].startsolid)
				testRayExtents = rayCheck.ray[i].m_Extents * 1.001; // prevent z-fighting
			// draw trace up to impact
			if (exceededThresholdFromStart[i]) {
				vdo->AddSweptBoxOverlay(corner[i], impact[i], -raySuccessMaxs, raySuccessMaxs, vec3_angle, 0, 255, 0, 100, duration);
				if (drawProperExtentsWithImpact)
					vdo->AddSweptBoxOverlay(corner[i], impact[i], -testRayExtents, testRayExtents, vec3_angle, 255, 255, 255, 100, duration);
			}
			// draw weight for both corners
			if (rayCheck.trace[i].startsolid)
				vdo->AddTextOverlay(corner[i], duration, "0.00"); // even tho the validation is -100 it's treated as 0
			else
				vdo->AddTextOverlay(corner[i], duration, "%d: %.2f + %.2f", rayCheck.cornerIdx[i], rayCheck.oldValidationVal[i], rayCheck.validationDelta[i]);
		}
		// part of the trace after the impact
		if (exceededThresholdFromEnd[0] || exceededThresholdFromEnd[1]) {
			vdo->AddSweptBoxOverlay(impact[0], impact[1], -rayFailMaxs, rayFailMaxs, vec3_angle, 255, 0, 0, 100, duration);
			// draw box at impact location
			for (int i = 0; i < 2; i++)
				if (exceededThresholdFromStart[i])
					vdo->AddBoxOverlay(impact[i], -impactMaxs, impactMaxs, vec3_angle, 0, 255, 0, 75, duration);
		}
		if (!drawProperExtentsWithImpact)
			vdo->AddSweptBoxOverlay(corner[0], corner[1], -testRayExtents, testRayExtents, vec3_angle, 255, 255, 255, 100, duration);
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
			curSubStepTime += std::fmax(0, hacks::curTime() - lastDrawTime); // prevents saveloads from messing up the animation
		}
		lastDrawTime = hacks::curTime();
		//double subStepFrac = std::fmin(1, curSubStepTime / subStepDurations[curStep]); // how far are we into the current substep, 0..1 range
		//Assert(subStepFrac >= 0);

		// If we have a fast enough animation speed it's possible to draw several substeps on the same frame.
		// This is a problem since the bbox of the ents get drawn every time (and have an alpha component), so just keep track if we've done that already.
		bool hasDrawnBBoxThisFrame = false;
		bool hasDrawnCollidedEntsThisFrame = false;
		for (;;) {
			Assert(curQueue);
			FcpsEvent* fe = curQueue->getEventWithId(curId);
			Assert(fe);
			auto& curLoop = fe->loops[curLoopIdx];
			// draw current substep
			if (shouldDrawStep[curStep]) {
				if (!hasDrawnBBoxThisFrame && curStep != AS_Revert && curStep != AS_Success) {
					vdo->AddBoxOverlay(curCenter, fe->origMins, fe->origMaxs, vec3_angle, 255, 255, 0, 25, dur); // original bounds
					// vdo->AddBoxOverlay(curCenter, curMins, curMaxs, vec3_angle, 255, 255, 0, 25, dur); // shrunk bounds
					hasDrawnBBoxThisFrame = true;
				}
				if (!hasDrawnCollidedEntsThisFrame) {
					for (int i = 0; i < fe->collidingEntsCount; i++) {
						auto& cEnt = fe->collidingEnts[i];
						vdo->AddBoxOverlay(cEnt.center, -cEnt.extents, cEnt.extents, cEnt.angles, 200, 0, 200, 25, dur);
						vdo->AddTextOverlay(cEnt.center, dur, "(%d) Name: %s (%s)", cEnt.entIdx, cEnt.debugName, cEnt.className);
					}
					hasDrawnCollidedEntsThisFrame = true;
				}
				switch (curStep) {
					case AS_TestTrace: {
						Ray_t& entRay = curLoop.entRay;
						trace_t& entTrace = curLoop.entTrace;
						Vector rayEnd = entRay.m_Start + curLoop.entRay.m_Delta;
						vdo->AddSweptBoxOverlay(entRay.m_Start, rayEnd, fe->origMins * 1.001, fe->origMaxs * 1.001, vec3_angle, 50, 200, 200, 0, dur);
						if (!curLoop.entTrace.startsolid)
							vdo->AddBoxOverlay(entTrace.endpos, fe->origMins, fe->origMaxs, vec3_angle, 0, 255, 0, 25, dur);
						break;
					}
					case AS_CornerValidation:
						for (int i = 0; i <= cornerIdx; i++)
							vdo->AddTextOverlay(curLoop.corners[i], dur, "%d: %s", i + 1, curLoop.cornersOob[i] ? "OOB" : "INBOUNDS");
						break;
					case AS_CornerRays:
						drawRayTest(dur);
						break;
					case AS_Revert:
						vdo->AddBoxOverlay(fe->origCenter, fe->origMins, fe->origMaxs, vec3_angle, 255, 0, 0, 50, dur);
						break;
					case AS_Success:
						vdo->AddBoxOverlay(fe->newCenter, fe->origMins, fe->origMaxs, vec3_angle, 0, 255, 0, 50, dur);
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
						curLoopIdx = 0; // fall through to skip the first TestTrace step (nothing happens in it)
					case AS_TestTrace:
						// after the trace, FCPS either succeeds or goes on to the rest of the loop
						if (fe->wasSuccess && curLoopIdx == fe->loopStartCount - 1) {
							curStep = AS_Success;
						} else {
							curStep = AS_CornerValidation;
							cornerIdx = curLoopIdx == 0 ? 0 : -1; // don't draw any traces on the first substep (but do the first loop)
						}
						break;
					case AS_CornerValidation:
						if (++cornerIdx >= 8) {
							// After checking all corners for validity, FCPS fires rays. Check if there's any rays to fire.
							for (curValidationRayIdx = 0; curValidationRayIdx < curLoop.validationRayCheckCount; curValidationRayIdx++) {
								auto& rayCheck = curLoop.validationRayChecks[curValidationRayIdx];
								if (!rayCheck.trace[0].startsolid || !rayCheck.trace[1].startsolid)
									break;
							}
							curStep = AS_CornerRays;
							if (curValidationRayIdx == curLoop.validationRayCheckCount)
								curValidationRayIdx = -1; // just show weights, no rays to fire
						}
						break;
					case AS_CornerRays:
						if (curValidationRayIdx != -1) {
							for (;;) {
								if (++curValidationRayIdx == curLoop.validationRayCheckCount) {
									curValidationRayIdx = -1;
									break;
								} else {
									auto& rayCheck = curLoop.validationRayChecks[curValidationRayIdx];
									if (!rayCheck.trace[0].startsolid || !rayCheck.trace[1].startsolid)
										break;
								}
							}
							break;
						}
						curStep = AS_NudgeAndAdjust; // fall through
						nextSubStepIsTrace = false;
					case AS_NudgeAndAdjust:
						if (nextSubStepIsTrace) {
							curStep = AS_TestTrace;
						} else if (curLoopIdx == 99) {
							curStep = AS_Revert;
						} else {
							curLoopIdx++;
							Vector oldCenter = curCenter;
							curCenter = fe->loops[curLoopIdx - 1].newCenter;
							curMins = fe->loops[curLoopIdx - 1].newMins;
							curMaxs = fe->loops[curLoopIdx - 1].newMaxs;
							nextSubStepIsTrace = true;
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
							curCenter = nextEvent->origCenter;
							curMins = nextEvent->adjustedMins;
							curMaxs = nextEvent->adjustedMaxs;
							hasDrawnBBoxThisFrame = false;
						}
						break;
				}
				// don't subtract time if we shouldn't be drawing this step
				if (!shouldDrawStep[curStep])
					curSubStepTime += subStepDurations[curStep];
			} else {
				break;
			}
		}
	}


	CON_COMMAND(un_stop_fcps_animation, "Stops any in-progress FCPS animation.") {
		stopFcpsAnimation();
	}


	void CC_Animate_Events(const CCommand& args, fcps::FixedFcpsQueue* fcpsQueue) {
		if (args.ArgC() < 2) {
			Msg("you must specify event IDs to animate\n");
			return;
		}
		unsigned long lower, upper;
		if (!fcpsQueue || !parseFcpsEventRange(args.Arg(1), lower, upper, fcpsQueue)) {
			Msg("\"%s\" is not a valid value or a valid range of values (check if events with the given values exist)\n", args.Arg(1));
			return;
		}
		fcpsAnimator.beginAnimation(lower, upper, un_fcps_animation_speed.GetFloat(), fcpsQueue);
	}


	CON_COMMAND(un_animate_recorded_fcps_events, "[x]|[x:y] - animates the FCPS events with the given ID or range of IDs, use un_fcps_animation_speed to specify the animation speed.") {
		CC_Animate_Events(args, RecordedFcpsQueue);
	}


	CON_COMMAND(un_animate_loaded_fcps_events, "[x]|[x:y] - animates the FCPS events with the given ID or range of IDs, use un_fcps_animation_speed to specify the animation speed.") {
		CC_Animate_Events(args, LoadedFcpsQueue);
	}


	CON_COMMAND(un_step_fcps_animation, "Sets the current FCPS animation to the next step.") {
		fcpsAnimator.stepAnimation();
	}

	void animation_speed_callback(IConVar* var, const char* pOldValue, float flOldValue) {
		if (un_fcps_animation_speed.GetFloat() < 0) {
			Msg("animation speed must be greater than or equal to 0\n");
			un_fcps_animation_speed.SetValue(pOldValue);
			return;
		}
		fcpsAnimator.adjustAnimationSpeed(un_fcps_animation_speed.GetFloat());
	}

	ConVar un_fcps_animation_speed("un_fcps_animation_speed", "20", FCVAR_NONE, "Sets the FCPS animation speed so that the total animation length lasts this many seconds, use 0 for manual step mode.", &animation_speed_callback);
}