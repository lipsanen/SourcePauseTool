#include "stdafx.h"

#include "fcps_memory_repr.hpp"
#include "..\OrangeBox\spt-serverplugin.hpp"
#include "..\miniz\miniz.h"
#include <chrono>
#include <thread>

// clang-format off

namespace fcps {

	FixedFcpsQueue* RecordedFcpsQueue = new FixedFcpsQueue(MAX_LOADED_EVENTS); // any new events are put here
	FixedFcpsQueue* LoadedFcpsQueue = nullptr; // any events loaded from disk are put here

	extern char* FcpsCallerNames[];
	
	// fixed queue

	
	FixedFcpsQueue::FixedFcpsQueue(int count) {
		arrSize = count;
		size = start = pushCount = 0;
		arr = new FcpsEvent[count];
	}


	FixedFcpsQueue::~FixedFcpsQueue() {
		for (int i = 0; i < size; i++)
			arr[(start + i) % arrSize].~FcpsEvent(); // we used placement new, so we need to explicitly call the destructor
		delete[] arr;
	}


	FcpsEvent& FixedFcpsQueue::beginNextEvent() {
		FcpsEvent& nextEvent = arr[(start + size) % arrSize];
		new (&nextEvent) FcpsEvent(++pushCount); // placement new; smallest ID is 1, increments from then on
		if (size == arrSize)
			start = (start + 1) % arrSize;
		if (size != arrSize)
			size++;
		return nextEvent;
	}


	FcpsEvent* FixedFcpsQueue::getLastEvent() {
		if (size)
			return &arr[(start + size - 1) % arrSize];
		return nullptr;
	}


	FcpsEvent* FixedFcpsQueue::getEventWithId(unsigned long id) {
		if (id < 1 || size == 0)
			return nullptr;
		int smallestId = arr[start].eventId;
		if (id < smallestId || id >= smallestId + size)
			return nullptr;
		return &arr[(start + id - smallestId) % arrSize];
	}


	int FixedFcpsQueue::count() {
		return size;
	}


	void FixedFcpsQueue::printAllEvents() {
		for (int i = 0; i < size; i++) {
			// sleep to minimize lines getting shuffled around
			std::this_thread::sleep_for(std::chrono::microseconds(10));
			arr[(start + i) % arrSize].print();
		}
	}


	// fcps event

	
	FcpsEvent::FcpsEvent(int eventId) : FcpsEvent() {
		this->eventId = eventId;
	}


	FcpsEvent::FcpsEvent(std::istream& infile) {
		infile.read((char*)this, sizeof(FcpsEvent));
	}


	void FcpsEvent::writeTextToDisk(FILE* f) {
		#define BOOL_STR(x) ((x) ? "true" : "false")
		#define DECOMPOSE(v) v.x, v.y, v.z
		#define FL_F "%.4f"
		#define VEC_F "<" FL_F ", " FL_F ", " FL_F ">"
		#define IND_1 "  "
		#define IND_2 IND_1 IND_1
		#define IND_3 IND_2 IND_1

		fprintf(f, "******************** FCPS Event %d ********************\n", eventId);
		fprintf(f, IND_1 "map: %s\n", mapName);
		fprintf(f, IND_1 "time: %d ticks (%.3fs)\n", tickTime, time);
		if (wasRunOnPlayer)
			fprintf(f, IND_1 "run on: (%d) %s\n", thisEnt.entIdx, thisEnt.debugName);
		else
			fprintf(f, IND_1 "run on: (%d) %s %s, held: %s\n", thisEnt.entIdx, thisEnt.debugName, thisEnt.className, BOOL_STR(isHeldObject));
		fprintf(f, IND_1 "mask: %d, collision group: %d\n", fMask, collisionGroup);
		fprintf(f, IND_1 "called from: %s\n", FcpsCallerNames[caller]);
		fprintf(f, IND_1 "vIndecisivePush: " VEC_F "\n", DECOMPOSE(vIndecisivePush));
		fprintf(f, IND_1 "original entity center: " VEC_F "\n", DECOMPOSE(origCenter));
		fprintf(f, IND_1 "entity extents (from center): " VEC_F "\n", DECOMPOSE(origMaxs));
		fprintf(f, IND_1 "ray extents and grow size: " VEC_F "\n", DECOMPOSE(growSize));
		for (int loopIdx = 0; loopIdx < loopStartCount; loopIdx++) {
			FcpsLoop& loopInfo = loops[loopIdx];
			fprintf(f, "ITERATION %d:\n", loopIdx + 1);
			if (loopIdx != 0) {
				float frac = loopInfo.entTrace.fraction;
				Vector deltaVec = loopInfo.entRay.m_Delta;
				float dist = (deltaVec * frac).Length();
				fprintf(f, IND_1 "tracing fat ray from new position to original, fraction traced: " FL_F ", distance: " FL_F " / " FL_F "\n", frac, dist, deltaVec.Length());
			}
			if (!loopInfo.entTrace.startsolid) {
				fprintf(f, "entity is no longer stuck, teleporting it to fat ray hit location\n");
				fprintf(f, "final center: " VEC_F "\n", DECOMPOSE(newCenter));
				fprintf(f, "final origin: " VEC_F "\n", DECOMPOSE(newOrigin));
				return;
			}
			fprintf(f, IND_1 "entity is still stuck, determining which corners are inbounds\n");
			bool anyInbounds = false;
			for (int cornerIdx = 0; cornerIdx < 8; cornerIdx++) {
				char locDescription[7];
				#define GET_SIGN_BIT(n) (cornerIdx & (1 << n))
				sprintf(locDescription, "%cx%cy%cz",  GET_SIGN_BIT(0) ? '+' : '-', GET_SIGN_BIT(1) ? '+' : '-',  GET_SIGN_BIT(2) ? '+' : '-');
				bool cornerValid = !loopInfo.cornersOob[cornerIdx];
				Vector actualLoc = loopInfo.entRay.m_Start;
				actualLoc.x += GET_SIGN_BIT(0) ? origMaxs.x : origMins.x;
				actualLoc.y += GET_SIGN_BIT(1) ? origMaxs.y : origMins.y;
				actualLoc.z += GET_SIGN_BIT(2) ? origMaxs.z : origMins.z;
				fprintf(f, IND_2 "corner %d (%s), inbounds: %s,%s location: " VEC_F ",  ray start: " VEC_F "\n", cornerIdx + 1, locDescription, BOOL_STR(cornerValid), cornerValid ? " " : "", DECOMPOSE(actualLoc), DECOMPOSE(loopInfo.corners[cornerIdx]));
				anyInbounds |= !loopInfo.cornersOob[cornerIdx];
			}
			if (anyInbounds) {
				fprintf(f, IND_1 "firing rays from every inbounds corner to every other corner\n");
				for (int twcIdx = 0; twcIdx < 28; twcIdx++) {
					auto& twc = loopInfo.twoWayRayChecks[twcIdx];
					for (int i = 0; i < 2; i++) {
						auto& owc = twc.checks[i];
						if (owc.trace.startsolid)
							continue;
						float frac = owc.trace.fraction;
						Vector deltaVec = owc.trace.endpos - owc.trace.startpos;
						float dist = (deltaVec * frac).Length();
						fprintf(f, IND_2 "corner %d to %d, fraction: " FL_F ", distance: " FL_F " / " FL_F "\n",twc.checks[i].cornerIdx + 1, twc.checks[1 - i].cornerIdx + 1, frac, dist, deltaVec.Length());
					}
				}
			}
			if (loopInfo.totalWeight > 0) {
				fprintf(f, IND_2 "total weights of each corner:\n");
				for (int cornerIdx = 0; cornerIdx < 8; cornerIdx++) {
					float weight = loopInfo.cornerWeights[cornerIdx];
					fprintf(f, IND_3 "corner %d: " FL_F "\n", cornerIdx + 1, weight < 0 ? 0 : weight);
				}
				fprintf(f, "pushing entity towards most-inbounds corners and increasing ray extents\n");
			} else {
				fprintf(f, IND_1 "no corners are valid, pushing entity towards vIndecisivePush and resetting ray extents\n");
			}
			fprintf(f, IND_1 "new ray extents: " VEC_F "\n", DECOMPOSE(loopInfo.newCornerRayExtents));
			fprintf(f, IND_1 "entity nudged by: " VEC_F "\n", DECOMPOSE(loopInfo.newOriginDirection));
			fprintf(f, IND_1 "new entity center: " VEC_F "\n", DECOMPOSE(loopInfo.newCenter));
		}
		fprintf(f, "100 iterations complete but entity is still stuck, reverting entity position to original location\n");
	}


	extern char* FcpsCallerNames[];

	void FcpsEvent::print() {
		Msg("ID: %d, map: %s, time: %4.3f, %d iteration%s (%s), called from %s, run on (%d) Name: %s (%s)\n",
			eventId,
			mapName,
			time,
			loopFinishCount,
			loopFinishCount == 1 ? "" : "s",
			wasSuccess ? "SUCCEEDED" : "FAILED",
			FcpsCallerNames[caller],
			thisEnt.entIdx,
			thisEnt.debugName,
			thisEnt.className);
	}


	// commands


	void showStoredEvents(FixedFcpsQueue* queue, char* verbStr) {
		if (!queue || !queue->count()) {
			Msg("No %s events\n", verbStr);
			return;
		}
		Msg("%d %s event%s:\n", queue->count(), verbStr, queue->count() == 1 ? "" : "s");
		queue->printAllEvents();
	}


	CON_COMMAND_F(fcps_show_recorded_events, "Prints all recorded FCPS events.", FCVAR_DONTRECORD) {
		showStoredEvents(RecordedFcpsQueue, "recorded");
	}


	CON_COMMAND_F(fcps_show_loaded_events, "Prints all recorded loaded events.", FCVAR_DONTRECORD) {
		showStoredEvents(LoadedFcpsQueue, "loaded");
	}


	// converts arg to a range, if arg is an int then lower = upper = int, if arg has the format int1-int2 then lower = int1 & upper = int2
	bool parseFcpsEventRange(const char* arg, unsigned long& lower, unsigned long& upper, FixedFcpsQueue* fcpsQueue) {
		// had trouble with scanf, hail my glorious parsing code
		char* end;
		upper = lower = strtoul(arg, &end, 10);
		if (end == arg) // check if lower number parsed at all
			return false;
		// now scan to see if upper exists
		bool isRange = false;
		bool upperParsed = false;
		char* curPtr = end; // curPtr is now after the first num
		while (*curPtr) {
			if (*curPtr == '-') {
				if (isRange)
					return false; // we already parsed a separator, this tells us there's a second one "x-y-..."
				isRange = true;
				curPtr++;
			} else if (isdigit(*curPtr)) {
				if (upperParsed || !isRange)
					return false;
				upper = strtoul(curPtr, &curPtr, 10);
				upperParsed = true;
			} else if (isspace(*curPtr)) {
				curPtr++;
			} else {
				return false;
			}
		}
		if ((isRange && !upperParsed) || lower > upper)
			return false;
		// check that events with these ID's exist, all IDs in between must also exist
		return fcpsQueue->getEventWithId(lower) && fcpsQueue->getEventWithId(upper);
	}


	CON_COMMAND_F(fcps_save_events, "[file] [x]|[x-y] (no extesion) - saves the event with ID x and writes it to the given file, use x-y to save a range of events (inclusive)", FCVAR_DONTRECORD) {
		if (args.ArgC() < 3) {
			Msg(" - %s\n", fcps_save_events_command.GetHelpText());
			return;
		}
		// check arg 2
		unsigned long lower, upper;
		if (!parseFcpsEventRange(args.Arg(2), lower, upper, RecordedFcpsQueue)) {
			Msg("\"%s\" is not a valid value or a valid range of values (check if events with the given values are recorded).\n", args.Arg(2));
			return;
		}
		std::string binstr = GetGameDir() + "\\" + args.Arg(1) + ".fcps";
		std::string txtstr = GetGameDir() + "\\" + args.Arg(1) + ".txt";
		remove(binstr.c_str());
		FILE* txtfile = fopen(txtstr.c_str(), "w");
		if (!txtfile) {
			Msg("Could not write to file \"%s\"\n", txtstr.c_str());
			return;
		}
		char version_str[20];
		int version_str_len = snprintf(version_str, 20, "version %d", FCPS_EVENT_VERSION);

		for (auto id = lower; id <= upper; id++) {
			char archive_name[20];
			sprintf(archive_name, "event %d", id);
			auto fcpsEvent = RecordedFcpsQueue->getEventWithId(id);
			Assert(fcpsEvent);
			if (!mz_zip_add_mem_to_archive_file_in_place(binstr.c_str(), archive_name, fcpsEvent, sizeof(FcpsEvent), version_str, version_str_len, MZ_DEFAULT_LEVEL)) {
				Msg("Could not write to file \"%s\"\n", binstr.c_str());
				fclose(txtfile);
				return;
			}
			fcpsEvent->writeTextToDisk(txtfile);
			if (id != upper)
				fprintf(txtfile, "\n\n");
		}
		fclose(txtfile);
		Msg("Successfully saved %d event%s to file.\n", upper - lower + 1, upper - lower == 0 ? "" : "s");
	}


	CON_COMMAND_F(fcps_load_events, "loads the specified .fcps file (no extension)", FCVAR_DONTRECORD) {

		#define CLEANUP {mz_zip_reader_end(&zip_archive); return;}
		#define BAD_FORMAT_EXIT {Msg("File \"%s\" does not have the correct format.\n", inpath); CLEANUP}
		#define DELETE_QUEUE_BAD_FORMAT {delete LoadedFcpsQueue; BAD_FORMAT_EXIT}

		if (args.ArgC() < 2) {
			Msg("- %s\n", fcps_load_events_command.GetHelpText());
			return;
		}
		std::string str = GetGameDir() + "\\" + args.Arg(1) + ".fcps";
		auto inpath = str.c_str();
		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));

		if (!mz_zip_reader_init_file(&zip_archive, inpath, 0)) {
			Msg("Could not open file \"%s\", it may have an incorrect format.\n", inpath);
			CLEANUP
		}
		mz_uint32 archive_count = zip_archive.m_total_files;
		if (archive_count > MAX_LOADED_EVENTS)
			BAD_FORMAT_EXIT
		stopFcpsAnimation();
		if (LoadedFcpsQueue)
			delete LoadedFcpsQueue;
		LoadedFcpsQueue = new FixedFcpsQueue(archive_count);

		for (mz_uint i = 0; i < archive_count; i++) {
			mz_zip_archive_file_stat file_stat;
			int version;
			if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)
				|| file_stat.m_is_directory
				|| !file_stat.m_is_supported
				|| sscanf(file_stat.m_comment, "version %d", &version) != 1)
			{
				DELETE_QUEUE_BAD_FORMAT
			}
			if (version != FCPS_EVENT_VERSION) {
				Msg("File \"%s\" is not the correct version, expected %d but got %d.\n", inpath, FCPS_EVENT_VERSION, version);
				delete LoadedFcpsQueue;
				CLEANUP
			}
			// check this after so that we get the correct error message
			if (file_stat.m_uncomp_size != sizeof(FcpsEvent))
				DELETE_QUEUE_BAD_FORMAT
			if (!mz_zip_reader_extract_file_to_mem(&zip_archive, file_stat.m_filename, &LoadedFcpsQueue->beginNextEvent(), sizeof(FcpsEvent), 0))
				DELETE_QUEUE_BAD_FORMAT
		}
		Msg("Successfully loaded %d events from file.\n", archive_count);
		CLEANUP
	}
}
