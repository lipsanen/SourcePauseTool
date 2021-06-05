#include "stdafx.h"

#include "fcps_memory_repr.hpp"

// clang-format off

namespace fcps {

	FixedFcpsQueue* RecordedFcpsQueue = new FixedFcpsQueue(200);
	FixedFcpsQueue* LoadedFcpsQueue = nullptr;

	
	// fixed queue

	
	FixedFcpsQueue::FixedFcpsQueue(int count) {
		arrSize = count;
		size = start = pushCount = 0;
		arr = new FcpsEvent[count];
	}


	FixedFcpsQueue::~FixedFcpsQueue() {
		delete[] arr;
	}


	FcpsEvent& FixedFcpsQueue::beginNextEvent() {
		if (size == arrSize)
			start = (start + 1) % arrSize;
		FcpsEvent& nextEvent = arr[(start + size) % arrSize];
		nextEvent.eventId = ++pushCount; // smallest ID is 1, increments from then on
		if (size != arrSize)
			size++;
		return nextEvent;
	}


	FcpsEvent* FixedFcpsQueue::getEventWithId(int id) {
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
		for (int i = 0; i < size; i++)
			arr[(start + i) % arrSize].print();
	}


	// fcps event


	FcpsEvent::FcpsEvent(std::istream& infile) {
		infile.read((char*)this, sizeof(FcpsEvent));
	}


	void FcpsEvent::writeToDisk(std::ofstream* outBinFile, std::ofstream* outTextFile) {
		if (outBinFile)
			outBinFile->write((char*)this, sizeof(FcpsEvent));
		if (outTextFile)
			Warning("no implementation of text representation yet\n");
	}


	void FcpsEvent::print() {
		Msg("ID: %2d, map: %-14s, time: %-4.3f, %2d iterations, %s, called from %s\n",
			eventId, mapName, curTime, totalFailCount, wasSuccess ? "SUCCEEDED" : "FAILED", FcpsCallerNames[caller]);
	}


	// commands


	void showStoredEvents(FixedFcpsQueue* queue, char* verbStr) {
		if (queue->count() == 0) {
			Msg("No %s events\n", verbStr);
			return;
		}
		Msg("%d %s event%s:\n", queue->count(), verbStr, queue->count() == 1 ? "" : "s");
		queue->printAllEvents();
	}


	CON_COMMAND_F(un_show_recorded_FCPS_events, "prints all recorded FCPS calls since un_store_FCPS_calls was set\n", FCVAR_CHEAT) {
		showStoredEvents(RecordedFcpsQueue, "recorded");
	}


	CON_COMMAND_F(un_show_loaded_FCPS_events, "prints all FCPS calls loaded with un_load_FCPS_events\n", FCVAR_CHEAT) {
		showStoredEvents(LoadedFcpsQueue, "loaded");
	}


	bool parseFcpsEventRange(const char* arg, int* lower, int* upper) {
		if (sscanf(arg, "%d:%d", lower, upper) != 2) {
			if (sscanf(arg, "%d", lower) == 1)
				*upper = *lower;
			else
				*lower = *upper = -1;
		}
		if (!RecordedFcpsQueue->getEventWithId(*lower) || !RecordedFcpsQueue->getEventWithId(*upper))
			return false;
		return true;
	}


	CON_COMMAND_F(un_save_FCPS_events, "[file] [x] or [file] [x:y] (no extesion) - saves the event with ID x and writes it to the given file, use x:y to save a range of events (inclusive)\n", FCVAR_CHEAT) {
		if (args.ArgC() < 3) {
			Msg("you must specify a file path and which events to save\n");
			return;
		}
		// check arg 2
		int lower, upper;
		if (!parseFcpsEventRange(args.Arg(2), &lower, &upper)) {
			Msg("\"%s\" is not a valid value or a valid range of values (check if events with the given values exist)\n", args.Arg(2));
			return;
		}
		// check arg 1
		std::string outpath = GetGameDir() + "\\" + args.Arg(1) + ".fcps";
		std::ofstream outfile(outpath, std::ios::binary | std::ios::out | std::ios::trunc);

		if (!outfile.is_open()) {
			Msg("could not create output file \"%s\"\n", outpath.c_str());
			return;
		}
		// now we can write the data
		uint32_t version = FCPS_EVENT_VERSION;
		outfile.write((char*)version, 4);
		for (int i = lower; i <= upper; i++)
			RecordedFcpsQueue->getEventWithId(i)->writeToDisk(&outfile, nullptr);
		Msg("Successfully wrote %d event%s to file\n", upper - lower + 1, upper - lower == 1 ? "" : "s");
	}


	CON_COMMAND_F(un_load_FCPS_events, "loads the specified .fcps file (no extension)\n", FCVAR_CHEAT) {
		if (args.ArgC() < 2) {
			Msg("you must specify a .fcps file");
			return;
		}
		std::ifstream infile(GetGameDir() + "\\" + args.Arg(1) + ".fcps", std::ios::binary | std::ios::ate | std::ios::in);
		if (!infile.is_open()) {
			Msg("could not open file\n");
			return;
		}
		int infileSize = infile.tellg();
		if (infileSize < 5 || (infileSize - 4) % sizeof(FcpsEvent) != 0) { // version number + events
			Msg("file appears to be the correct format (incorrect size)\n");
			return;
		}
		infile.seekg(0);
		int32_t version;
		infile.read((char*)&version, 4);
		if (version != FCPS_EVENT_VERSION) {
			Msg("file does not appear to be the correct version, expected version %d but got %d\n", FCPS_EVENT_VERSION, version);
			return;
		}
		stopFcpsAnimation();
		if (LoadedFcpsQueue)
			delete LoadedFcpsQueue;
		int numEvents = (infileSize - sizeof(int)) / sizeof(FcpsEvent);
		LoadedFcpsQueue = new FixedFcpsQueue(numEvents);
		for (int _ = 0; _ < numEvents; _++)
			LoadedFcpsQueue->beginNextEvent() = FcpsEvent(infile);
		Msg("%d events loaded from file\n", numEvents);
	}
}
