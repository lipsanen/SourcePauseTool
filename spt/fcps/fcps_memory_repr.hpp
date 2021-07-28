#pragma once

//#include "fcps_override.hpp"
#include <fstream>
#define GAME_DLL
#include "cbase.h"

// clang-format off

// contains objects to store the FCPS algorithm in memory and read/write it from disk

namespace fcps {

	#define MAX_LOADED_EVENTS 200
	
	enum FcpsCaller;
	
	// if anything in these structs or file writing is changed, the event version must be updated
	#define FCPS_EVENT_VERSION 8

	#define MAP_NAME_LEN 64
	#define MAX_COLLIDING_ENTS 10

	struct EntInfo {
		int entIdx;
		char debugName[256];
		char className[256];
		Vector center, extents; // extents are from cetner, not axis-aligned
		QAngle angles;
	};

	// one big struct, everything is in-place (no pointers) so that saving/loading is super easy
	struct FcpsEvent {

		// general info
		int eventId;
		char mapName[MAP_NAME_LEN];
		FcpsCaller caller;
		float time;
		int tickTime;
		bool wasRunOnPlayer;
		bool isHeldObject;
		// if this wasn't run on the player, we probably want to see where the player was
		EntInfo playerInfo;
		Vector vIndecisivePush;
		int fMask;
		
		EntInfo thisEnt; // the entity that fcps was called on
		Vector origCenter, origOrigin;
		Vector origMins, origMaxs; // from center
		int collisionGroup;
		Vector growSize;
		Vector rayStartMins, rayStartMaxs; // extents of where rays are fired from

		EntInfo collidingEnts[MAX_COLLIDING_ENTS]; // ents that get hit by rays from this event, only used for animation
		int collidingEntsCount;

		struct FcpsLoop {
			uint failCount;
			Ray_t entRay;
			trace_t entTrace;
			// the rest is only valid in case of ent trace failure
			Vector corners[8]; // the extents can get modified on every iteration so we need to keep track of that
			bool cornersOob[8];

			// the result of a ray fired from a->b and b->a
			struct TwoWayRayCheck {
				// the result of a ray fired from a->b
				struct OneWayRayCheck {
					int cornerIdx;
					// the ray/trace is only valid if the corner is inbounds since rays are only fired then
					// (the exception is that the startsolid flag of the trace is always valid)
					Ray_t ray;
					trace_t trace;
					float oldWeight;
					float weightDelta;
				} checks[2];
			} twoWayRayChecks[28];

			float cornerWeights[8];
			float totalWeight;
			Vector newOriginDirection;
			Vector newCenter, newMins, newMaxs; // new ent extents (from center)
			Vector newCornerRayExtents;
		} loops[100];

		int loopStartCount;
		int loopFinishCount;
		bool wasSuccess;
		// success or failure
		Vector newOrigin, newCenter;

		FcpsEvent() = default;
		FcpsEvent(int eventId);
		~FcpsEvent() = default;
		FcpsEvent(std::istream& infile);
		void writeTextToDisk(FILE* f);
		void print(); // should all fit on one line
	};


	// circular queue with fixed size
	class FixedFcpsQueue {
	private:
		int arrSize, size, start, pushCount;
		FcpsEvent* arr;
	public:
		FixedFcpsQueue(int count);
		~FixedFcpsQueue();
		FcpsEvent& beginNextEvent(); // returns with ID field set
		FcpsEvent* getLastEvent();
		FcpsEvent* getEventWithId(unsigned long id); // returns null if there is no such event
		void printAllEvents();
		int count();
	};

	
	extern FixedFcpsQueue* RecordedFcpsQueue;
	extern FixedFcpsQueue* LoadedFcpsQueue;

	// Similiar to python/C# ranges: "1^-3" means all IDs from 1 to third to last inclusive.
	// Kinda weird choice of characters, but I don't want to use ':' as a separator since
	// that forces you to use quotes, and "1-^3" feels less intuitive to me.
	#define RANGE_SEP_STR "^"
	#define RANGE_SEP_CHAR RANGE_SEP_STR[0]
	#define RANGE_NEG_STR "-"
	#define RANGE_NEG_CHAR RANGE_NEG_STR[0]
	#define RANGE_HELP_STR "use x" RANGE_SEP_STR "y to specify a range of events (inclusive). Negative values for x or y are relative from the end of the list."

	extern void stopFcpsAnimation();
	bool parseFcpsEventRange(const char* arg, unsigned long& lower, unsigned long& upper, FixedFcpsQueue* fcpsQueue); // parses "x" or "x^y" into a range, returns true on success
}
