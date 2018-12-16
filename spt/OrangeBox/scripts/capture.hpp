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
	private:
		std::vector<CaptureData> captureData;
		int currentTick;
		std::string currentCmd;
	};

	extern Capture g_Capture;
}