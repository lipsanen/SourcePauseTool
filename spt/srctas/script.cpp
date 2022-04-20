#include "stdafx.h"
#include "script.hpp"
#include "utils.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace srctas
{
	const int SCRIPT_VERSION = 1;

	static inline void ltrim(std::string& s)
	{
		s.erase(s.begin(),
		        std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
	}

	static inline void rtrim(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
		        s.end());
	}

	static inline void trim(std::string& s)
	{
		ltrim(s);
		rtrim(s);
	}

	void Script::Clear()
	{
		m_vFrameBulks.clear();
	}

	Error Script::WriteToFile(const char* filepath)
	{
		Error error;
		std::ofstream stream;
		stream.open(filepath);

		if (!stream.is_open())
		{
			if (filepath)
				error.m_sMessage = format("Unable to open filepath: %s", filepath);
			else
				error.m_sMessage = format("Unable to open filepath: %s", "null");

			error.m_bError = true;
			return error;
		}

		stream << "version " << SCRIPT_VERSION << std::endl;
		stream << "frames" << std::endl;

		for (auto& framebulk : m_vFrameBulks)
		{
			stream << framebulk.GetFramebulkString() << std::endl;
		}

		return error;
	}

	enum class ScriptParserState
	{
		FileNotOpen,
		Preamble,
		Frames,
		Done
	};

	struct ScriptParser
	{
		ScriptParserState m_eState;
		Script* m_sOutput;
		Error m_sError;
		const char* m_sFilepath;
		bool m_bHasLine;
		std::string m_sLine;
		std::ifstream m_fStream;
		std::size_t m_uLineIndex;
		int m_iVersion;

		ScriptParser()
		{
			m_eState = ScriptParserState::FileNotOpen;
			m_bHasLine = false;
			m_uLineIndex = 0;
			m_iVersion = -1;
		}

		void OpenFile();
		void ReadLine();
		void ParseIteration();
		void ParsePreamble();
		void ParseFrames();
	};

	Error Script::ParseFrom(const char* filepath, Script* output)
	{
		ScriptParser parser;

		if (output == nullptr)
		{
			parser.m_sError.m_bError = true;
			parser.m_sError.m_sMessage = "Output pointer was null";
			return parser.m_sError;
		}

		output->Clear();

		parser.m_sOutput = output;
		parser.m_sFilepath = filepath;

		while (parser.m_eState != ScriptParserState::Done && !parser.m_sError.m_bError)
			parser.ParseIteration();

		if (parser.m_sError.m_bError)
		{
			std::string message = parser.m_sError.m_sMessage.c_str();
			if (message.c_str())
				parser.m_sError.m_sMessage =
				    format("Error parsing file on line %u: %s", parser.m_uLineIndex, message.c_str());
			else
				parser.m_sError.m_sMessage =
				    format("Error parsing file on line %u: %s", parser.m_uLineIndex, "null");
		}

		return parser.m_sError;
	}

	void ScriptParser::ParseIteration()
	{
		if (m_eState == ScriptParserState::FileNotOpen)
		{
			OpenFile();
		}
		else if (!m_bHasLine)
		{
			ReadLine();
		}
		else
		{
			if (m_eState == ScriptParserState::Preamble)
				ParsePreamble();
			else if (m_eState == ScriptParserState::Frames)
				ParseFrames();
			m_bHasLine = false;
		}
	}

	void ScriptParser::OpenFile()
	{
		if (m_sFilepath == nullptr || *m_sFilepath == '\0')
		{
			m_sError.m_bError = true;
			m_sError.m_sMessage = "Filepath was empty";
			return;
		}

		m_fStream.open(m_sFilepath);

		if (!m_fStream.good())
		{
			m_sError.m_bError = true;
			m_sError.m_sMessage = format("Unable to open file %s", m_sFilepath);
		}
		else
		{
			m_eState = ScriptParserState::Preamble;
		}
	}

	void ScriptParser::ReadLine()
	{
		m_sLine.clear();
		std::getline(m_fStream, m_sLine);
		++m_uLineIndex;

		if (!m_fStream.good() && m_sLine.empty())
		{
			if (m_eState != ScriptParserState::Frames)
			{
				m_sError.m_bError = true;
				m_sError.m_sMessage = "File does not look like a src2tas script.";
			}

			m_eState = ScriptParserState::Done;
		}
		else
		{
			// Trim out any comments
			auto commentIndex = m_sLine.find("//");

			if (commentIndex != std::string::npos)
			{
				m_sLine.resize(commentIndex);
			}

			// Trim extra spaces
			trim(m_sLine);

			m_bHasLine = !m_sLine.empty();
		}
	}

	void ScriptParser::ParsePreamble()
	{
		std::istringstream iss;
		iss.str(m_sLine);

		std::string word1;
		std::string word2;
		iss >> word1;

		if (word1 == "frames")
		{
			if (m_iVersion == -1)
			{
				m_sError.m_bError = true;
				m_sError.m_sMessage = "Version number was not specified!";
			}
			else
			{
				m_eState = ScriptParserState::Frames;
				return;
			}
		}
		else if (word1 == "version")
		{
			iss >> word2;
			m_iVersion = std::atoi(word2.c_str());

			if (m_iVersion <= 0 || m_iVersion > SCRIPT_VERSION)
			{
				m_sError.m_bError = true;
				m_sError.m_sMessage = format("Src2tas version %d not supported", m_iVersion);
				return;
			}
		}
		else
		{
			m_sError.m_bError = true;
			m_sError.m_sMessage = format("Invalid keyword %s", word1.c_str());
			return;
		}
	}

	void ScriptParser::ParseFrames()
	{
		FrameBulkError error;
		FrameBulk newBulk = FrameBulk::Parse(m_sLine, error);

		if (error.m_bError)
		{
			m_sError.m_bError = true;
			m_sError.m_sMessage = error.m_sMessage;
		}
		else
		{
			m_sOutput->m_vFrameBulks.push_back(newBulk);
		}
	}
} // namespace srctas