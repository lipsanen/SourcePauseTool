#include "stdafx.h"

#include "fcps_memory_repr.hpp"
#include "..\OrangeBox\spt-serverplugin.hpp"

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
		for (int i = 0; i < size; i++)
			arr[(start + i) % arrSize].~FcpsEvent(); // we used placement new, so we need to explicitly call the destructor
		delete[] arr;
	}


	FcpsEvent& FixedFcpsQueue::beginNextEvent() {
		if (size == arrSize)
			start = (start + 1) % arrSize;
		FcpsEvent& nextEvent = arr[(start + size) % arrSize];
		new (&nextEvent) FcpsEvent(++pushCount); // placement new; smallest ID is 1, increments from then on
		if (size != arrSize)
			size++;
		return nextEvent;
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
		for (int i = 0; i < size; i++)
			arr[(start + i) % arrSize].print();
	}


	// fcps event

	
	FcpsEvent::FcpsEvent(int eventId) : eventId(eventId) {}


	FcpsEvent::FcpsEvent(std::istream& infile) {
		infile.read((char*)this, sizeof(FcpsEvent));
	}


	void FcpsEvent::writeToDisk(std::ofstream* outBinFile, std::ofstream* outTextFile) {
		if (outBinFile)
			outBinFile->write((char*)this, sizeof(FcpsEvent));
		if (outTextFile)
			Warning("no implementation of text representation yet\n");
	}


	extern char* FcpsCallerNames[];

	void FcpsEvent::print() {
		Msg("ID: %2d, map: %-14s, time: %-4.3f, %2d iterations, %s, called from %s\n",
			eventId, mapName, curTime, totalFailCount, wasSuccess ? "SUCCEEDED" : "FAILED", FcpsCallerNames[caller]);
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


	CON_COMMAND(un_show_recorded_fcps_events, "Prints all recorded FCPS events.") {
		showStoredEvents(RecordedFcpsQueue, "recorded");
	}


	CON_COMMAND(un_show_loaded_fcps_events, "Prints all recorded loaded events.") {
		showStoredEvents(LoadedFcpsQueue, "loaded");
	}


	// converts arg to a range, if arg is an int then lower = upper = int, if arg has the format int1:int2 then lower = int1 & upper = int2
	bool parseFcpsEventRange(const char* arg, unsigned long& lower, unsigned long& upper, FixedFcpsQueue* fcpsQueue) {
		// I tried scanf %d:%d and that didn't work so maybe idk how scanf works, hail my glorious parsing code
		char* end;
		upper = lower = strtol(arg, &end, 10);
		if (end == arg) // check if lower number parsed at all
			return false;
		// now scan to see if upper exists
		bool isRange = false;
		bool upperParsed = false;
		char* curPtr = end; // curPtr is now after the first num
		while (*curPtr) {
			if (*curPtr == ':') {
				if (isRange)
					return false; // we alredy parsed a second digit, this tells us there's a second separator "x:y:..."
				isRange = true;
				curPtr++;
			} else if (isdigit(*curPtr)) {
				if (upperParsed || !isRange)
					return false;
				upper = strtol(curPtr, &curPtr, 10);
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
		return fcpsQueue->getEventWithId(lower) && !fcpsQueue->getEventWithId(upper);
	}


	CON_COMMAND(un_save_fcps_events, "[file] [x]|[x:y] (no extesion) - saves the event with ID x and writes it to the given file, use x:y to save a range of events (inclusive)") {
		if (args.ArgC() < 3) {
			Msg("You must specify a file path and which events to save.\n");
			return;
		}
		// check arg 2
		unsigned long lower, upper;
		if (!parseFcpsEventRange(args.Arg(2), lower, upper, RecordedFcpsQueue)) {
			Msg("\"%s\" is not a valid value or a valid range of values (check if events with the given values are recorded).\n", args.Arg(2));
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
		outfile.write((char*)&version, 4);
		for (int i = lower; i <= upper; i++)
			RecordedFcpsQueue->getEventWithId(i)->writeToDisk(&outfile, nullptr);
		Msg("Successfully wrote %d event%s to file.\n", upper - lower + 1, upper - lower == 0 ? "" : "s");
	}


	CON_COMMAND(un_load_fcps_events, "loads the specified .fcps file (no extension)") {
		if (args.ArgC() < 2) {
			Msg("you must specify a .fcps file\n");
			return;
		}

		std::ifstream infile(GetGameDir() + "\\" + args.Arg(1) + ".fcps", std::ios::binary | std::ios::ate | std::ios::in);
		if (!infile.is_open()) {
			Msg("Could not open file.\n");
			return;
		}
		int infileSize = infile.tellg();
		if (infileSize < 5 || (infileSize - 4) % sizeof(FcpsEvent) != 0) { // version number + events
			Msg("File appears to be the incorrect format (incorrect size).\n");
			return;
		}
		infile.seekg(0);
		int32_t version;
		infile.read((char*)&version, 4);
		if (version != FCPS_EVENT_VERSION) {
			Msg("File does not appear to be the correct version, expected version %d but got %d.\n", FCPS_EVENT_VERSION, version);
			return;
		}
		stopFcpsAnimation();
		if (LoadedFcpsQueue)
			delete LoadedFcpsQueue;
		int numEvents = (infileSize - sizeof(int)) / sizeof(FcpsEvent);
		LoadedFcpsQueue = new FixedFcpsQueue(numEvents);
		for (int _ = 0; _ < numEvents; _++)
			new (&LoadedFcpsQueue->beginNextEvent()) FcpsEvent(infile); // placement new
		Msg("%d event%s loaded from file\n", numEvents, numEvents == 1 ? "" : "s");
	}
}
