#include "stdafx.h"

#include <string>
#include "fcps_animation.hpp"
#include "fcps_override.hpp"
#include "..\OrangeBox\spt-serverplugin.hpp"

// clang-format off

namespace fcps {

	// it shouldn't really matter what this value is, but it should be used for manual step stuff for consistency
	#define MANUAL_STEP_DURATION 1.0


	FcpsAnimator fcpsAnimator = FcpsAnimator(); // global animator object


	FcpsAnimator::FcpsAnimator() : curStep(AS_NotAnimating) {}


	FcpsAnimator::~FcpsAnimator() {}


	void FcpsAnimator::stopAnimation() {
		curStep = AS_NotAnimating;
	}


	void FcpsAnimator::beginAnimation(int from, int to, double seconds, FixedFcpsQueue* fcpsQueue) {
		isSetToManualStep = seconds == 0;
		curSubStepTime = isSetToManualStep ? MANUAL_STEP_DURATION : 0; // in manual step I always want to be at the very end of the current sub step
		curQueue = fcpsQueue;
		fromId = curId = from;
		toId = to;
		curQueue = fcpsQueue;
		calcSubStepDurations(seconds);
		curStep = AS_DrawBBox;
		lastDrawTime = hacks::curTime();
		lastDrawFrame = hacks::frameCount();
		lastDrawTick = hacks::tickCount();

		// substep specific vars
		curLoopIdx = 0;
		cornerIdx = 0;
		curTwcIdx = 0;
		FcpsEvent* nextEvent = curQueue->getEventWithId(from);
		curCenter = nextEvent->origCenter;
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
			
			for (int loopIdx = 0; loopIdx < fe->loopFinishCount; loopIdx++) {
				auto& loop = fe->loops[loopIdx];
				for (int twcIdx = 0; twcIdx < 28; twcIdx++) {
					auto& twc = loop.twoWayRayChecks[twcIdx];
					if (!loop.cornersOob[twc.checks[0].cornerIdx] || !loop.cornersOob[twc.checks[1].cornerIdx])
						subStepCounts[AS_CornerRays]++; // one/two-way ray check substep from every inbounds corner
				}
				subStepCounts[AS_CornerRays]++; // +1 for one substep to show all validations
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
		if (!isAnimating())
			return;
		double oldSubStepFrac = curSubStepTime / subStepDurations[curStep];
		calcSubStepDurations(seconds);
		if (isSetToManualStep)
			curSubStepTime = MANUAL_STEP_DURATION;
		else
			curSubStepTime = oldSubStepFrac * subStepDurations[curStep];
	}


	bool FcpsAnimator::canManualStep() const {
		if (!isAnimating()) {
			Msg("No animation in progress.\n");
			return false;
		}
		if (!isSetToManualStep) {
			Msg("Current animation is not in step mode.\n");
			return false;
		}
		return true;
	}


	void FcpsAnimator::stepAnimation() {
		if (canManualStep()) {
			// keep the substep time at the end of the substep in manual step mode
			if (curSubStepTime == 0)
				curSubStepTime = MANUAL_STEP_DURATION;
			else
				curSubStepTime = 2 * MANUAL_STEP_DURATION;
		}
	}


	void FcpsAnimator::skipStepType() {
		if (canManualStep())
			skipUntilNextStep = true;
	}


	void FcpsAnimator::setHeldStepButton(bool pressed) {
		if (stepButtonHeld = pressed) {
			stepAnimation();
			secondsSinceLastHeldStep = 0;
		}
	}


	bool FcpsAnimator::isEventCurrentlyAnimated(int eventId) const {
		return isAnimating() && eventId == curId;
	}


	bool FcpsAnimator::isAnimating() const {
		return curStep != AS_NotAnimating;
	}


	void stopFcpsAnimation() {
		fcpsAnimator.stopAnimation();
	}


	bool isFcpsEventCurrentlyAnimated(int eventId) {
		return fcpsAnimator.isEventCurrentlyAnimated(eventId);
	}


	void FcpsAnimator::drawRaysFromCorners(float duration) const {

		auto vdo = GetDebugOverlay();
		auto& loopInfo = curQueue->getEventWithId(curId)->loops[curLoopIdx];

		// last substep - don't draw rays, just draw the total validation values
		if (curTwcIdx == 28) {
			for (int i = 0; i < 8; i++)
				vdo->AddTextOverlay(loopInfo.corners[i], duration, "%d: %.2f", i + 1, loopInfo.cornerWeights[i]);
			return;
		}

		auto& twc = loopInfo.twoWayRayChecks[curTwcIdx];

		// at least one ray should have been fired
		Assert(!loopInfo.cornersOob[twc.checks[0].cornerIdx] || !loopInfo.cornersOob[twc.checks[1].cornerIdx]);
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
			corner[i] = loopInfo.corners[twc.checks[i].cornerIdx];
			impact[i] = twc.checks[i].trace.startsolid ? corner[i] : twc.checks[i].trace.endpos;
		}

		// check if the ray travelled at least a tiny bit (and didn't get all the way to the other corner)
		for (int i = 0; i < 2; i++) {
			exceededThresholdFromStart[i] = (corner[i] - impact[i]).LengthSqr() > distThresholdSqr;
			exceededThresholdFromEnd[i] = (corner[(i+1)%2] - impact[i]).LengthSqr() > distThresholdSqr;
		}
		Vector testRayExtents = loopInfo.rayExtents * 1.001; // prevent z-fighting
		for (int i = 0; i < 2; i++) {
			// draw trace up to impact
			if (exceededThresholdFromStart[i]) {
				vdo->AddSweptBoxOverlay(corner[i], impact[i], -raySuccessMaxs, raySuccessMaxs, vec3_angle, 0, 255, 0, 100, duration);
				if (drawProperExtentsWithImpact)
					vdo->AddSweptBoxOverlay(corner[i], impact[i], -testRayExtents, testRayExtents, vec3_angle, 255, 255, 255, 100, duration);
			}
			// draw weight for both corners
			if (twc.checks[i].weightDelta > 0)
				vdo->AddTextOverlay(corner[i], duration, "%d: %.2f + %.2f", twc.checks[i].cornerIdx + 1, twc.checks[i].oldWeight, twc.checks[i].weightDelta);
			else
				vdo->AddTextOverlay(corner[i], duration, "%d: 0.00", twc.checks[i].cornerIdx + 1);
		}
		// part of the trace after the impact
		if (exceededThresholdFromEnd[0] || exceededThresholdFromEnd[1]) {
			vdo->AddSweptBoxOverlay(impact[0], impact[1], -rayFailMaxs, rayFailMaxs, vec3_angle, 255, 0, 0, 100, duration);
			// draw box at impact location
			for (int i = 0; i < 2; i++)
				if (exceededThresholdFromStart[i])
					vdo->AddBoxOverlay(impact[i], -impactMaxs, impactMaxs, vec3_angle, 0, 255, 0, 75, duration);
		}
		if (!drawProperExtentsWithImpact || (!exceededThresholdFromStart[0] && !exceededThresholdFromStart[1]))
			vdo->AddSweptBoxOverlay(corner[0], corner[1], -testRayExtents, testRayExtents, vec3_angle, 255, 255, 255, 100, duration);
	}


	// this should be called at least every frame (no drawing is done again if this is called twice in a frame)
	void FcpsAnimator::draw() {
		if (!isAnimating())
			return;
		// XXX This should work if we call this method every frame but for some reason drawn stuff will bleed to the next call.
		// It would be nice if this worked because there is some occasional flickering and I think drawing every frame might fix that.
		if (hacks::frameCount() == lastDrawFrame || hacks::tickCount() == lastDrawTick)
			return;
		lastDrawFrame = hacks::frameCount();
		lastDrawTick = hacks::tickCount();
		auto vdo = GetDebugOverlay();
		if (!vdo)
			return;
		float dur = NDEBUG_PERSIST_TILL_NEXT_SERVER; // negative duration should make this only draw for one frame but it seem to work
		// at least 0 to prevent saveloads from messing up the animation
		float timeSinceLastDrawCall = std::fmax(0, hacks::curTime() - lastDrawTime);
		if (isSetToManualStep) {
			if (stepButtonHeld && (secondsSinceLastHeldStep += timeSinceLastDrawCall) > secondsPerHeldStep) {
				stepAnimation();
				secondsSinceLastHeldStep = fmodf(secondsSinceLastHeldStep, timeSinceLastDrawCall);
			}
		} else {
			curSubStepTime += timeSinceLastDrawCall;
		}
		lastDrawTime = hacks::curTime();
		//double subStepFrac = std::fmin(1, curSubStepTime / subStepDurations[curStep]); // how far are we into the current substep, 0..1 range
		//Assert(subStepFrac >= 0);

		AnimationStep origStep = curStep;

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
			if (shouldDrawStep[curStep] && !skipUntilNextStep) {
				if (!hasDrawnBBoxThisFrame) {
					if (curStep != AS_Revert && curStep != AS_Success)
						vdo->AddBoxOverlay(curCenter, fe->origMins, fe->origMaxs, vec3_angle, 255, 200, 25, 35, dur); // bounds that the alg uses
					vdo->AddBoxOverlay(curCenter, -fe->thisEnt.extents * 0.999f, fe->thisEnt.extents * 0.999f, fe->thisEnt.angles, 200, 200, 200, 10, dur); // actual entity bbox
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
						drawRaysFromCorners(dur);
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
			// this is > instead of >= to make the sure manual step doesn't draw 2 steps on the first draw
			if (curSubStepTime - subStepDurations[curStep] > 0 || !shouldDrawStep[curStep] || skipUntilNextStep) {
				if (shouldDrawStep[curStep] && !skipUntilNextStep)
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
							hasDrawnBBoxThisFrame = true;
							curCenter = fe->newCenter;
							curStep = AS_Success;
						} else {
							curStep = AS_CornerValidation;
							cornerIdx = curLoopIdx == 0 ? 0 : -1; // don't draw any traces on the first substep (but do on the first loop)
						}
						break;
					case AS_CornerValidation:
						if (++cornerIdx < 8)
							break;
						curStep = AS_CornerRays;
						curTwcIdx = -1; // fall through
						curTwcCount = 0;
					case AS_CornerRays:
						if (curTwcIdx < 28) {
							curTwcCount++;
							while (++curTwcIdx < 28) {
								auto& twc = curLoop.twoWayRayChecks[curTwcIdx];
								if (!curLoop.cornersOob[twc.checks[0].cornerIdx] || !curLoop.cornersOob[twc.checks[1].cornerIdx])
									break;
							}
							break;
						}
						curStep = AS_NudgeAndAdjust; // fall through
						nextSubStepIsTrace = false;
					case AS_NudgeAndAdjust:
						if (nextSubStepIsTrace) {
							curStep = AS_TestTrace;
						} else if (curLoopIdx == 99) {
							hasDrawnBBoxThisFrame = true;
							curCenter = fe->origCenter;
							curStep = AS_Revert;
						} else {
							curLoopIdx++;
							Vector oldCenter = curCenter;
							curCenter = fe->loops[curLoopIdx - 1].newCenter;
							nextSubStepIsTrace = true;
							if (curCenter.DistToSqr(oldCenter) > 1)
								hasDrawnBBoxThisFrame = false; // allow redrawning of bbox on this frame if it's in a slightly different location
						}
						break;
					case AS_Success:
					case AS_Revert:
						if (++curId > toId) {
							curStep = AS_NotAnimating;
							skipUntilNextStep = false;
							return;
						} else {
							curStep = AS_DrawBBox;
							FcpsEvent* nextEvent = curQueue->getEventWithId(curId);
							curCenter = nextEvent->origCenter;
							hasDrawnBBoxThisFrame = false;
						}
						break;
				}
				if (curStep != origStep)
					skipUntilNextStep = false;
				// don't subtract time if we shouldn't be drawing this step
				if (!shouldDrawStep[curStep] && !skipUntilNextStep)
					curSubStepTime += subStepDurations[curStep];
			} else {
				break;
			}
		}
	}

	std::wstring FcpsAnimator::getProgressString() const {
		if (!isAnimating())
			return L"no FCPS animation in progress";
		std::wstring pStr = L"event " + std::to_wstring(curId);
		if (fromId != toId)
			pStr += L" (" + std::to_wstring(curId - fromId + 1) + L"/" + std::to_wstring(toId - fromId + 1) + L")";
		pStr += L": loop " + std::to_wstring(curLoopIdx + 1) + L"/" + std::to_wstring(curQueue->getEventWithId(curId)->loopStartCount);
		if (curStep != AS_DrawBBox && curStep != AS_CornerValidation)
			pStr += L", ";
		switch (curStep) {
			case AS_TestTrace:
				pStr += L"new -> start pos ray";
				break;
			case AS_CornerValidation:
				if (cornerIdx != -1)
					pStr += L", is corner " + std::to_wstring(cornerIdx + 1) + L" inbounds?";
				break;
			case AS_CornerRays:
				if (curTwcIdx != 28) {
					auto& curLoop = curQueue->getEventWithId(curId)->loops[curLoopIdx];
					int twcCount = 0;
					for (int i = 0; i < 28; i++) {
						auto& twc = curLoop.twoWayRayChecks[i];
						if (!curLoop.cornersOob[twc.checks[0].cornerIdx] || !curLoop.cornersOob[twc.checks[1].cornerIdx])
							twcCount++;
					}
					pStr += L"tracing rays " + std::to_wstring(curTwcCount) + L"/" + std::to_wstring(twcCount);
				} else {
					pStr += L"showing corner weights";
				}
				break;
			case AS_NudgeAndAdjust:
				pStr += L"nudging entity";
				break;
			case AS_Success:
				pStr += L"FCPS success";
				break;
			case AS_Revert:
				pStr += L"FCPS fail";
				break;
		}
		return pStr;
	}


	CON_COMMAND_F(fcps_stop_animation, "Stops any in-progress FCPS animation.", FCVAR_DONTRECORD) {
		stopFcpsAnimation();
	}


	void CC_Animate_Events(const ConCommand& cmd, const CCommand& args, fcps::FixedFcpsQueue* fcpsQueue) {
		if (args.ArgC() < 2) {
			Msg(" - %s\n", cmd.GetHelpText());
			return;
		}
		unsigned long lower, upper;
		if (!fcpsQueue || !parseFcpsEventRange(args.Arg(1), lower, upper, fcpsQueue)) {
			Msg("\"%s\" is not a valid value or a valid range of values (check if events with the given values exist)\n", args.Arg(1));
			return;
		}
		fcpsAnimator.beginAnimation(lower, upper, fcps_animation_time.GetFloat(), fcpsQueue);
	}


	CON_COMMAND_F(fcps_animate_recorded_events, "[x]|[x" RANGE_SEP_STR "y] - animates the FCPS events with the given ID or range of IDs, " RANGE_HELP_STR " Use fcps_animation_time to specify the length of the animation.", FCVAR_DONTRECORD) {
		CC_Animate_Events(fcps_animate_recorded_events_command, args, RecordedFcpsQueue);
	}


	CON_COMMAND_F(fcps_animate_loaded_events, "[x]|[x" RANGE_SEP_STR "y] - animates the FCPS events with the given ID or range of IDs, " RANGE_HELP_STR " Use fcps_animation_time to specify the length of the animation.", FCVAR_DONTRECORD) {
		CC_Animate_Events(fcps_animate_loaded_events_command, args, LoadedFcpsQueue);
	}


	CON_COMMAND_F(fcps_animate_last_recorded_event, "Animates the last recorded event, use fcps_animation_time to specify the length of the animation.", FCVAR_DONTRECORD) {
		if (!RecordedFcpsQueue || !RecordedFcpsQueue->getLastEvent()) {
			Msg("No recorded events!\n");
			return;
		}
		int id = RecordedFcpsQueue->getLastEvent()->eventId;
		fcpsAnimator.beginAnimation(id, id, fcps_animation_time.GetFloat(), RecordedFcpsQueue);
	}


	CON_COMMAND_F(fcps_skip_current_step_type, "Fast-forwards the animation to the next step type.", FCVAR_DONTRECORD) {
		fcpsAnimator.skipStepType();
	}


	CON_COMMAND_F(fcps_step_animation, "Sets the current FCPS animation to the next step.", FCVAR_DONTRECORD) {
		fcpsAnimator.stepAnimation();
	}


	ConCommand fcps_step_press_command("+fcps_step_animation", [](const CCommand&) {fcpsAnimator.setHeldStepButton(true);}, "Bind +fcps_step_animation to step while holding a key.", FCVAR_DONTRECORD);
	ConCommand fcps_step_release_command("-fcps_step_animation", [](const CCommand&) {fcpsAnimator.setHeldStepButton(false);}, "Bind +fcps_step_animation to step while holding a key.", FCVAR_DONTRECORD);


	void animation_speed_callback(IConVar* var, const char* pOldValue, float flOldValue) {
		if (fcps_animation_time.GetFloat() < 0) {
			Msg("animation speed must be greater than or equal to 0\n");
			fcps_animation_time.SetValue(pOldValue);
			return;
		}
		fcpsAnimator.adjustAnimationSpeed(fcps_animation_time.GetFloat());
	}

	ConVar fcps_animation_time("fcps_animation_time", "20", FCVAR_DONTRECORD, "Sets the FCPS animation speed so that the total animation length lasts this many seconds, use 0 for manual step mode.", &animation_speed_callback);


	ConVar fcps_hud_progress("fcps_hud_progress", "0", FCVAR_DONTRECORD, "If enabled, displays the state of the current playing FCPS animation.");
}