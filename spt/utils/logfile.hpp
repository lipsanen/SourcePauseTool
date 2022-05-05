#pragma once

#include <string>
#include <fstream>
#include "platform.hpp"

class __declspec(dllexport) LogFile
{
private:
	std::string m_sFilepath;
	std::ofstream m_outputStream;
public:
	void Log(const char* filepath, const char* fmt, ...);
	~LogFile() {};
	LogFile()
	{
		m_outputStream.close();
	};
};