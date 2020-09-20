#include "stdafx.h"
#include "variable_container.hpp"
#include "..\cvars.hpp"
#include "dbg.h"
#include "srctas_reader.hpp"
#include "spt\ipc\ipc-spt.hpp"

namespace scripts
{
	const int FAIL_TICK = -1;

	void VariableContainer::PrintBest()
	{
		if (lastSuccessPrint.empty())
		{
			Msg("No results found.\n");
		}
		else
		{
			Msg("Best result was with:\n");
			Msg(lastSuccessPrint.c_str());
		}
	}

	void VariableContainer::Clear()
	{
		variableMap.clear();
		lastResult = SearchResult::NoSearch;
		lastSuccessPrint.clear();
		iterationPrint.clear();
		lastSuccessTick = FAIL_TICK;
	}

	void VariableContainer::Iteration(SearchType type)
	{
		int maxChanges;
		int changes = 0;
		searchType = type;

		if (type == SearchType::IPC)
		{
			PromptIPC();
			return;
		}
		else if (type == SearchType::None)
			maxChanges = 0;
		else if (type == SearchType::Highest || type == SearchType::Lowest || type == SearchType::Range)
			maxChanges = 1;
		else
			maxChanges = variableMap.size();

		for (auto& var : variableMap)
		{
			if (var.second.Iteration(lastResult, type))
				++changes;

			if (changes > maxChanges)
			{
				if (type == SearchType::None)
					throw std::exception("Not in search mode, range variables are illegal");
				else
					throw std::exception("Binary search only accepts one range variable");
			}
		}
	}

	void VariableContainer::AddNewVariable(const std::string& type,
	                                       const std::string& name,
	                                       const std::string& value)
	{
		auto newVar = ScriptVariable(type, value);
		variableMap[name] = newVar;
	}

	void VariableContainer::SetResult(SearchResult result)
	{
		if (searchType == SearchType::None)
			throw std::exception("Set result while not in search mode");

		SendIPCResult(result);

		// If in IPC search or not in search mode, don't repeat the search
		if (result == SearchResult::NoSearch)
			throw SearchDoneException();

		lastResult = result;

		if (Successful(result))
		{
			lastSuccessTick = g_TASReader.GetCurrentTick();
			lastSuccessPrint = iterationPrint + "\t- tick: " + std::to_string(lastSuccessTick) + "\n";
			if (searchType == SearchType::Random)
				throw SearchDoneException();
		}
	}

	void VariableContainer::PrintState()
	{
		if (variableMap.size() == 0)
			return;

		iterationPrint = "Variables:\n";
		for (auto& var : variableMap)
		{
			iterationPrint += "\t - " + var.first + " : " + var.second.GetPrint() + "\n";
		}

		if (tas_script_printvars.GetBool())
		{
			Msg(iterationPrint.c_str());
		}
	}

	void VariableContainer::ReadIPCVariables(const nlohmann::json& msg)
	{
		if (msg.find("variables") == msg.end())
		{
			throw std::exception("Variable response did not contain a variables property!");
		}

		for (auto& variable : variableMap)
		{
			if (variable.second.GetVariableType() == VariableType::Var)
				continue;

			if (msg["variables"].find(variable.first) == msg["variables"].end())
			{
				const char* string =
				    FormatTempString("Variable %s missing from IPC message", variable.first.c_str());
				throw std::exception(string);
			}

			auto& value = msg["variables"][variable.first];

			if (!value.is_number_integer())
			{
				const char* string =
				    FormatTempString("Variable %s value was not an integer", variable.first.c_str());
				throw std::exception(string);
			}

			variable.second.SetIndex(value);
		}
	}

	nlohmann::json VariableContainer::GetVariablesJson()
	{
		nlohmann::json variables;
		for (auto& variable : variableMap)
		{
			auto type = variable.second.GetVariableType();
			if (type == VariableType::Var)
				continue;

			variables[variable.first] = nlohmann::json();
			auto& variableData = variable.second.GetVariableData();

			switch (type)
			{
			case VariableType::AngleRange:
				variables[variable.first]["type"] = "angle";
				break;
			case VariableType::FloatRange:
				variables[variable.first]["type"] = "float";
				break;
			case VariableType::IntRange:
				variables[variable.first]["type"] = "int";
				break;
			default:
				throw std::exception(
				    FormatTempString("Got error type in variable %s when prompting IPC variables",
				                     variable.first.c_str()));
			}

			switch (type)
			{
			case VariableType::AngleRange:
			case VariableType::FloatRange:
				variables[variable.first]["lowIndex"] = 0;
				variables[variable.first]["highIndex"] = variableData.floatRange.GetHighIndex();
				variables[variable.first]["high"] = variableData.floatRange.GetHigh();
				variables[variable.first]["low"] = variableData.floatRange.GetLow();
				variables[variable.first]["increment"] = variableData.floatRange.GetIncrement();
				variables[variable.first]["index"] = variableData.floatRange.GetIndex();
				variables[variable.first]["value"] = variableData.floatRange.GetValue();
				break;
			case VariableType::IntRange:
				variables[variable.first]["lowIndex"] = 0;
				variables[variable.first]["highIndex"] = variableData.intRange.GetHighIndex();
				variables[variable.first]["high"] = variableData.intRange.GetHigh();
				variables[variable.first]["low"] = variableData.intRange.GetLow();
				variables[variable.first]["increment"] = variableData.intRange.GetIncrement();
				variables[variable.first]["index"] = variableData.intRange.GetIndex();
				variables[variable.first]["value"] = variableData.intRange.GetValue();
				break;
			}
		}

		return variables;
	}

	bool VariableContainer::Successful(SearchResult result)
	{
		if (result != SearchResult::Success)
			return false;
		else if (searchType == SearchType::RandomHighest)
		{
			if (lastSuccessTick == FAIL_TICK)
				return true;
			else
				return g_TASReader.GetCurrentTick() > lastSuccessTick;
		}
		else if (searchType == SearchType::RandomLowest)
		{
			if (lastSuccessTick == FAIL_TICK)
				return true;
			else
				return g_TASReader.GetCurrentTick() < lastSuccessTick;
		}

		return true;
	}

	void VariableContainer::PromptIPC()
	{
		if (!ipc::IsActive())
		{
			throw std::exception("Tried to use IPC search when IPC client is not connected.\n");
		}

		nlohmann::json msg;
		msg["type"] = "variables";
		msg["variables"] = GetVariablesJson();

		ipc::RemoveMessagesFromQueue("variables");
		ipc::Send(msg);
		bool result = ipc::BlockFor("variables");
		if (!result)
		{
			throw std::exception("IPC variable request timed out.\n");
		}
	}

	void VariableContainer::SendIPCResult(SearchResult result)
	{
		if (ipc::IsActive() && searchType == SearchType::IPC)
		{
			nlohmann::json msg;
			msg["type"] = "searchResult";
			msg["success"] = result == SearchResult::Success;
			msg["variables"] = GetVariablesJson();
			msg["tick"] = g_TASReader.GetCurrentTick();
			ipc::Send(msg);
		}
	}

	ScriptVariable::ScriptVariable(const std::string& type, const std::string& value)
	{
		if (type == "var")
		{
			variableType = VariableType::Var;
			data.value = value;
		}
		else if (type == "int")
		{
			variableType = VariableType::IntRange;
			data.intRange.ParseInput(value, false);
		}
		else if (type == "float")
		{
			variableType = VariableType::FloatRange;
			data.floatRange.ParseInput(value, false);
		}
		else if (type == "angle")
		{
			variableType = VariableType::AngleRange;
			data.floatRange.ParseInput(value, true);
		}
		else
			throw std::exception("Unknown typename for variable");
	}

	std::string ScriptVariable::GetPrint()
	{
		switch (variableType)
		{
		case VariableType::Var:
			return data.value;
		case VariableType::IntRange:
			return data.intRange.GetRangeString();
		case VariableType::FloatRange:
		case VariableType::AngleRange:
			return data.floatRange.GetRangeString();
		default:
			throw std::exception("Unexpected variable type while printing");
		}
	}

	std::string ScriptVariable::GetValue()
	{
		switch (variableType)
		{
		case VariableType::Var:
			return data.value;
		case VariableType::IntRange:
			return data.intRange.GetValue();
		case VariableType::FloatRange:
		case VariableType::AngleRange:
			return data.floatRange.GetValue();
		default:
			throw std::exception("Unexpected variable type while getting value");
		}
	}

	bool ScriptVariable::Iteration(SearchResult search, SearchType type)
	{
		switch (variableType)
		{
		case VariableType::Var:
			return false;
		case VariableType::IntRange:
			data.intRange.Select(search, type);
			return true;
		case VariableType::FloatRange:
		case VariableType::AngleRange:
			data.floatRange.Select(search, type);
			return true;
		default:
			throw std::exception("Unexpected variable type while starting iteration");
		}
	}
	void ScriptVariable::SetIndex(int index)
	{
		if (GetVariableType() == VariableType::IntRange)
		{
			data.intRange.SetIndex(index);
		}
		else if (GetVariableType() == VariableType::FloatRange || GetVariableType() == VariableType::AngleRange)
		{
			data.floatRange.SetIndex(index);
		}
		else
		{
			throw std::exception("Tried to set the value of a non-mutable variable!");
		}
	}
	const VarData& ScriptVariable::GetVariableData()
	{
		return data;
	}
	VariableType ScriptVariable::GetVariableType()
	{
		return variableType;
	}
} // namespace scripts
