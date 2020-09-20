#include "thirdparty\json.hpp"

namespace ipc
{
	void Init();
	bool IsActive();
	void Loop();
	void Send(const nlohmann::json& msg);
	bool BlockFor(std::string msg);
	void RemoveMessagesFromQueue(const std::string& type);
	void ShutdownIPC();
} // namespace ipc
