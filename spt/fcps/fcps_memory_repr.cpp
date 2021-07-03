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


	void FcpsEvent::writeTextToDisk(std::ofstream* outTextFile) {
		if (outTextFile)
			Warning("no implementation of text representation yet\n");
	}


	extern char* FcpsCallerNames[];

	void FcpsEvent::print() {
		Msg("ID: %d, map: %s, time: %4.3f, %d iteration%s (%s), called from %s, run on (%d) Name: %s (%s)\n",
			eventId,
			mapName,
			curTime,
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
					return false; // we already parsed a separator, this tells us there's a second one "x:y:..."
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
		return fcpsQueue->getEventWithId(lower) && fcpsQueue->getEventWithId(upper);
	}


	CON_COMMAND(un_save_fcps_events, "[file] [x]|[x:y] (no extesion) - saves the event with ID x and writes it to the given file, use x:y to save a range of events (inclusive)") {
		if (args.ArgC() < 3) {
			Msg("You must specify a file path and which events to save.\n - %s\n", un_save_fcps_events_command.GetHelpText());
			return;
		}
		// check arg 2
		unsigned long lower, upper;
		if (!parseFcpsEventRange(args.Arg(2), lower, upper, RecordedFcpsQueue)) {
			Msg("\"%s\" is not a valid value or a valid range of values (check if events with the given values are recorded).\n", args.Arg(2));
			return;
		}
		std::string str = GetGameDir() + "\\" + args.Arg(1) + ".fcps";
		auto outpath = str.c_str();
		remove(outpath);
		char version_str[20];
		int version_str_len = snprintf(version_str, 20, "version %d", FCPS_EVENT_VERSION);

		for (auto id = lower; id <= upper; id++) {
			char archive_name[20];
			sprintf(archive_name, "event %d", id);
			auto fcpsEvent = RecordedFcpsQueue->getEventWithId(id);
			Assert(fcpsEvent);
			mz_bool status = mz_zip_add_mem_to_archive_file_in_place(outpath, archive_name, fcpsEvent, sizeof(FcpsEvent), version_str, version_str_len, MZ_DEFAULT_LEVEL);
			if (!status) {
				Msg("Could not write to file \"%s\"\n", outpath);
				return;
			}
		}
		Msg("Successfully wrote %d event%s to file.\n", upper - lower + 1, upper - lower == 0 ? "" : "s");
	}


	CON_COMMAND(un_load_fcps_events, "loads the specified .fcps file (no extension)") {
		if (args.ArgC() < 2) {
			Msg("you must specify a .fcps file\n");
			return;
		}
		std::string str = GetGameDir() + "\\" + args.Arg(1) + ".fcps";
		auto inpath = str.c_str();
		mz_zip_archive zip_archive;
		memset(&zip_archive, 0, sizeof(zip_archive));

		if (!mz_zip_reader_init_file(&zip_archive, inpath, 0)) {
			Msg("Could not open file \"%s\", it may have an incorrect format.\n", inpath);
			goto cleanup;
		}
		mz_uint32 archive_count = zip_archive.m_total_files;
		if (archive_count > MAX_LOADED_EVENTS) {
			Msg("File \"%s\" does not have the correct format.\n", inpath);
			goto cleanup;
		}
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
				|| file_stat.m_uncomp_size != sizeof(FcpsEvent)
				|| sscanf(file_stat.m_comment, "version %d", &version) != 1)
			{
				Msg("File \"%s\" does not have the correct format.\n", inpath);
				delete LoadedFcpsQueue;
				goto cleanup;
			}
			if (version != FCPS_EVENT_VERSION) {
				Msg("File \"%s\" is not the correct version, expected %d but got %d.\n", FCPS_EVENT_VERSION, version);
				delete LoadedFcpsQueue;
				goto cleanup;
			}
			mz_zip_reader_extract_file_to_mem(&zip_archive, file_stat.m_filename, &LoadedFcpsQueue->beginNextEvent(), sizeof(FcpsEvent), 0);
		}
		Msg("Successfully loaded %d events from file.\n", archive_count);
	cleanup:
		mz_zip_reader_end(&zip_archive);
		return;
	}
}
