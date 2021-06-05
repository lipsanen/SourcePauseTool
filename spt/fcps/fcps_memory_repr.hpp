#pragma once

#include "fcps_override.hpp"
#include <fstream>

// clang-format off

// contains objects to store the FCPS algorithm in memory and read/write it from disk

namespace fcps {

	#define FCPS_EVENT_VERSION 1


	// if ANYTHING in this struct is changed, the event version must be updated

	struct FcpsEvent {
		int eventId; // ID
		char mapName[20];
		FcpsCaller caller;

		// general info
		int tickTime;
		float curTime;
		bool wasRunOnPlayer;
		char entName[120];
		Vector playerMins, playerMaxs; // in case this was not run on the player
		int fMask;
		
		// pre-loop
		Vector entMins, entMaxs;
		Vector originalCenter;
		Vector zNudgedCenter;
		int collisionGroup;
		Vector growSize;
		Vector adjustedMins, adjustedMaxs;


		struct FcpsLoop {
			int failCount;
			Vector rayStart, rayDelta;
			Ray_t testRayResult;
			bool wasSuccess;
			// the rest of this is only valid in case of failure
			Vector corners[8];
			bool cornersInbounds[8];

			struct ValidationCheck {
				int fromCorner, toCorner;
				Ray_t ray;
				trace_t trace;
				float validation;
			};

			int validationCheckCount;
			ValidationCheck validationChecks[56];

			Vector newOriginDirection;
			float totalValidation;
			Vector newCenter, newMins, newMaxs; // new Extents?
		};

		// all of the loops
		int totalFailCount;
		FcpsLoop loops[100];
		bool wasSuccess;
		// in case of success
		Vector newPos;

		FcpsEvent() = default;
		FcpsEvent(std::istream& infile);
		void writeToDisk(std::ofstream* outBinFile, std::ofstream* outTextFile);
		void print(); // should all fit on one line
	};


	// circular queue of fixed size
	class FixedFcpsQueue {
	private:
		int arrSize, size, start, pushCount;
		FcpsEvent* arr;
	public:
		FixedFcpsQueue(int count);
		~FixedFcpsQueue();
		FcpsEvent& beginNextEvent(); // returns with ID field set
		FcpsEvent* getEventWithId(int id); // returns null if there is no such event
		void printAllEvents();
		int count();
	};

	
	extern FixedFcpsQueue* RecordedFcpsQueue; // any new events are put here
	extern FixedFcpsQueue* LoadedFcpsQueue;   // any events loaded from disk are put here


	extern void stopFcpsAnimation();
	bool parseFcpsEventRange(const char* arg, int* lower, int* upper); // parses "x" or "x:y" into a range, returns true on success
}
