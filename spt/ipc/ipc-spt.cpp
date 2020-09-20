#include "stdafx.h"

#include "ipc-spt.hpp"

#include "spt\OrangeBox\scripts\srctas_reader.hpp"
#include "..\sptlib-wrapper.hpp"
#include "convar.h"
#include "ipc.hpp"

namespace ipc
{
	static ipc::IPCServer server;
#ifdef OE
	void IPC_Changed(ConVar* var, char const* pOldString);
#else
	void IPC_Changed(IConVar* var, const char* pOldValue, float flOldValue);
#endif
	ConVar y_spt_ipc("y_spt_ipc", "0", 0, "Enables IPC.", IPC_Changed);
	ConVar y_spt_ipc_port("y_spt_ipc_port", "27182", 0, "Port used for IPC.");
	ConVar y_spt_ipc_timeout("y_spt_ipc_timeout",
	                         "50",
	                         0,
	                         "Timeout in msec for blocking IPC reads.",
	                         true,
	                         1,
	                         true,
	                         1000,
	                         nullptr);
	ConVar y_spt_ipc_ack("y_spt_ipc_ack", "1", 0, "SPT sends acknowledgements to successful IPC commands\n");

	static void SendAck()
	{
		if (y_spt_ipc_ack.GetBool())
		{
			nlohmann::json ackMsg;
			ackMsg["type"] = "ack";
			ipc::Send(ackMsg);
		}
	}

	void StartIPC()
	{
		if (!ipc::Winsock_Initialized())
		{
			ipc::InitWinsock();
			server.StartListening(y_spt_ipc_port.GetString());
		}
	}

	void ShutdownIPC()
	{
		if (ipc::Winsock_Initialized())
		{
			server.CloseConnections();
			ipc::Shutdown_IPC();
		}
	}

	void CmdCallback(const nlohmann::json& msg)
	{
		SendAck();
		if (msg.find("cmd") != msg.end())
		{
			std::string cmd = msg["cmd"];
			EngineConCmd(cmd.c_str());
		}
		else
		{
			Msg("Message contained no cmd field!\n");
		}
	}

	void MsgWrapper(const char* msg)
	{
		Msg(msg);
	}

	void ReadVariables(const nlohmann::json& msg)
	{
		SendAck();
		scripts::g_TASReader.ReadIPCVariables(msg);
	}

	void Init()
	{
		ipc::AddPrintFunc(MsgWrapper);
		if (y_spt_ipc.GetBool())
		{
			StartIPC();
		}
		server.AddCallback("cmd", CmdCallback, false);
		server.AddCallback("variables", ReadVariables, true);
	}

	bool IsActive()
	{
		return ipc::Winsock_Initialized() && server.ClientConnected();
	}

	void Loop()
	{
		if (ipc::Winsock_Initialized())
		{
			server.Loop();
		}
	}

	void Send(const nlohmann::json& msg)
	{
		if (ipc::IsActive())
		{
			server.SendMsg(msg);
		}
	}

	bool BlockFor(std::string msg)
	{
		return server.BlockForMessages(msg, y_spt_ipc_timeout.GetInt());
	}

	void RemoveMessagesFromQueue(const std::string& type)
	{
		server.RemoveMessagesFromQueue(type);
	}

#ifdef OE
	void IPC_Changed(ConVar* var, char const* pOldString)
#else
	void IPC_Changed(IConVar* var, const char* pOldValue, float flOldValue)
#endif
	{
		if (y_spt_ipc.GetBool())
		{
			StartIPC();
		}
		else
		{
			ipc::ShutdownIPC();
		}
	}

} // namespace ipc
