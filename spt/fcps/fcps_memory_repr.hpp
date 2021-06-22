#pragma once

//#include "fcps_override.hpp"
#include <fstream>
#define GAME_DLL
#include "cbase.h"

// clang-format off

// contains objects to store the FCPS algorithm in memory and read/write it from disk

namespace fcps {
	enum FcpsCaller;

	#define FCPS_EVENT_VERSION 1

	#define MAP_NAME_LEN 64
	#define ENT_CLASS_NAME_LEN 256

	// if ANYTHING in this struct is changed, the event version must be updated

	struct FcpsEvent {

		// general info
		int eventId;
		char mapName[MAP_NAME_LEN];
		FcpsCaller caller;
		int tickTime;
		float curTime;
		bool wasRunOnPlayer;
		char entClassName[ENT_CLASS_NAME_LEN];
		bool isHeldObject;
		// if this wasn't run on the player, we probably want to see where the player was
		Vector playerMins, playerMaxs;
		Vector vIndecisivePush;
		int fMask;
		
		// pre-loop
		Vector entMins, entMaxs;
		Vector originalCenter;
		Vector zNudgedCenter;
		int collisionGroup;
		Vector growSize;
		Vector adjustedMins, adjustedMaxs;


		struct FcpsLoop {
			uint failCount;
			Ray_t testRay;
			trace_t testTraceResult;
			// the rest of this is only valid in case of failure
			Vector corners[8]; // the extents can get modified on every iteration so wee need to keep track of that
			bool cornersOob[8];

			struct ValidationRayCheck {
				int cornerIdx[2];
				Ray_t ray[2];
				trace_t trace[2];
				float validationDelta[2];
			};

			int validationRayCheckCount;
			ValidationRayCheck validationRayChecks[28];

			float cornerValidation[8];
			float totalValidation;
			Vector newOriginDirection;
			Vector newCenter, newMins, newMaxs; // new Extents
		};

		// all of the loops
		int loopStartCount;
		int loopFinishCount;
		FcpsLoop loops[100];
		bool wasSuccess;
		// in case of success
		Vector newPos;

		FcpsEvent() = default;
		FcpsEvent(int eventId);
		~FcpsEvent() = default;
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
		FcpsEvent* getEventWithId(unsigned long id); // returns null if there is no such event
		void printAllEvents();
		int count();
	};

	
	extern FixedFcpsQueue* RecordedFcpsQueue;
	extern FixedFcpsQueue* LoadedFcpsQueue;


	extern void stopFcpsAnimation();
	bool parseFcpsEventRange(const char* arg, unsigned long& lower, unsigned long& upper, FixedFcpsQueue* fcpsQueue); // parses "x" or "x:y" into a range, returns true on success
}
