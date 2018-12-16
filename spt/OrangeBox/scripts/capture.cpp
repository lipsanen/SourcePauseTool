#include "stdafx.h"
#include "capture.hpp"
#include "tier0\commonmacros.h"
#include "..\cvars.hpp"
#include "..\modules\EngineDLL.hpp"
#include "..\modules.hpp"
#include "framebulk_handler.hpp"
#include "..\..\sptlib-wrapper.hpp"
#include "srctas_reader.hpp"

namespace scripts
{
	Capture g_Capture;

	Capture::Capture()
	{
		currentTick = 0;
	}

	void Capture::SendCommand(const char* pText)
	{
		if (tas_recording.GetBool())
			currentCmd += pText;
	}

	void Capture::SendViewAngles(float yaw, float pitch)
	{
		if (tas_recording.GetBool())
		{
			std::replace(currentCmd.begin(), currentCmd.end(), '\n', ';');
			CaptureData data;
			data.cmd = currentCmd;
			data.length = 1;
			data.pitch = pitch;
			data.yaw = yaw;

			captureData.push_back(data);
			currentCmd.clear();
			++currentTick;
		}
	}

	void Capture::StartCapture()
	{
		float frameTime = (1 / tas_record_fps.GetFloat()) * tas_record_speed.GetFloat();
		if (frameTime > engineDLL.GetTickrate())
		{
			Msg("Frametime cannot be longer than engine ticks! Lower the recording speed or make the fps higher.\n");
			return;
		}

		if (fileName.empty())
		{
			Msg("Capture not properly set up!\n");
			return;
		}

		currentTick = 0;
		captureData.clear();
		char buffer[128];
		sprintf_s(buffer, ARRAYSIZE(buffer), "sv_cheats 1; host_framerate %.8f; y_spt_cvar fps_max %.8f", frameTime, tas_record_fps.GetFloat());
		EngineConCmd(buffer);

		try
		{
			auto empty = GetEmptyBulk(0, "");
			clientDLL.AddIntoAfterframesQueue(afterframes_entry_t(0, empty.outputData.getInitialCommand() + ";_y_spt_resetpitchyaw; tas_recording 1"));
		}
		catch (const std::exception& ex)
		{
			Msg("Starting the capture failed: %s\n", ex.what());
		}
	}

	void Capture::StopCapture(bool keep)
	{
		if (fileName.empty())
		{
			Msg("No capture to save.");
			return;
		}

		if (keep)
		{		
			WriteToFile();
		}

		Msg("Capture lasted %d ticks.\n", currentTick);
		Reset();
	}

	void Capture::CollapseDuplicates()
	{
		for (int i = captureData.size() - 2; i >= 0; --i)
		{
			if (captureData[i].CanCollapse(captureData[i + 1]))
			{
				Collapse(i);
			}
		}
	}

	void Capture::Collapse(int index)
	{
		captureData[index].length += captureData[index + 1].length;
		captureData.erase(captureData.begin() + index + 1);
	}

	void Capture::WriteToFile()
	{
		try
		{
			std::ofstream stream(fileName);
			CollapseDuplicates();
			auto& script = g_TASReader.GetCurrentScript();
			script.WriteScriptToStream(stream, startTick, "tas_cvars_reset");

			for (auto capture : captureData)
				stream << capture.ToString() << '\n';
		}
		catch(const std::exception& ex)
		{
			Msg("Unable to write capture to file, error: %s\n", ex.what());
		}
	}

	void Capture::SetupCapture(int tick, const std::string& fileName)
	{
		startTick = tick;
		this->fileName = fileName;
	}

	void Capture::Reset()
	{
		captureData.clear();
		currentTick = -1;
		currentCmd = "";
		startTick = 0;
		fileName.clear();
		tas_recording.SetValue(0);
	}


	bool CaptureData::CanCollapse(const CaptureData & rhs) const
	{
		return yaw == rhs.yaw && pitch == rhs.pitch && rhs.cmd == cmd;
	}

	std::string CaptureData::ToString()
	{
		return GenerateBulkString('>', length, cmd, yaw, pitch);
	}

}