#pragma once
#include <vector>

namespace scripts
{
	struct CaptureData
	{
		float yaw;
		float pitch;
		std::string cmd;
		int length;

		bool CanCollapse(const CaptureData& rhs) const;
		std::string ToString();
	};

	class Capture
	{
	public:
		Capture();
		void SendCommand(const char* pText);
		void SendViewAngles(float yaw, float pitch);
		void StartCapture();
		void StopCapture(bool keep);
		void CollapseDuplicates();
		void Collapse(int index);
		void WriteToFile();
		void SetupCapture(int tick, const std::string& fileName);
		void Reset();
	private:
		std::vector<CaptureData> captureData;
		int currentTick;
		std::string currentCmd;

		int startTick;
		std::string fileName;
	};

	extern Capture g_Capture;
}