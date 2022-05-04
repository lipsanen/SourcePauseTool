#pragma once
#include "..\feature.hpp"
#include <fstream>

// TAS logging
class TASLoggingFeature : public FeatureWrapper<TASLoggingFeature>
{
public:
	void Log(const char* fmt, ...);
	bool OpenStream();
protected:
	virtual bool ShouldLoadFeature() override;

	virtual void InitHooks() override;

	virtual void LoadFeature() override;

	virtual void UnloadFeature() override;

	std::string m_sFilepath;
	std::ofstream m_outputStream;
};

extern TASLoggingFeature spt_taslogging;
