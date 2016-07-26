#ifndef BBQUE_AGENT_PROXY_GRPC_H_
#define BBQUE_AGENT_PROXY_GRPC_H_

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include <grpc++/support/time.h>

#include "bbque/utils/logging/logger.h"
#include "bbque/plugins/agent_proxy_if.h"
#include "bbque/plugin_manager.h"
#include "bbque/plugins/plugin.h"
#include "agent_server.h"
#include "agent_client.h"

#define MODULE_NAMESPACE AGENT_PROXY_NAMESPACE".grpc"
#define MODULE_CONFIG AGENT_PROXY_CONFIG

namespace bbque
{
namespace plugins
{

using bbque::agent::ExitCode_t;

/**
 * @class AgentProxyGRPC
 *
 */
class AgentProxyGRPC: public bbque::plugins::AgentProxyIF
{
public:

	AgentProxyGRPC();

	virtual ~AgentProxyGRPC();

	// --- Plugin required
	static void * Create(PF_ObjectParams *);

	static int32_t Destroy(void *);
	// ---

	void WaitForServerToStop();

	// ----------------- Query status functions --------------------

	ExitCode_t GetResourceStatus(
	        std::string const & resource_path,
	        agent::ResourceStatus & status) override;


	ExitCode_t GetWorkloadStatus(
	        std::string const & path, agent::WorkloadStatus & status) override;

	ExitCode_t GetWorkloadStatus(
	        int system_id, agent::WorkloadStatus & status) override;


	ExitCode_t GetChannelStatus(
	        std::string const & path, agent::ChannelStatus & status) override;

	ExitCode_t GetChannelStatus(
	        int system_id, agent::ChannelStatus & status) override;


	// ------------- Multi-agent management functions ------------------

	ExitCode_t SendJoinRequest(std::string const & path) override;

	ExitCode_t SendJoinRequest(int system_id) override;


	ExitCode_t SendDisjoinRequest(std::string const & path) override;

	ExitCode_t SendDisjoinRequest(int system_id) override;


	// ----------- Scheduling / Resource allocation functions ----------

	ExitCode_t SendScheduleRequest(
	        std::string const & path,
	        agent::ApplicationScheduleRequest const & request) override;

private:

	std::string server_address_port = "0.0.0.0:";

	static uint32_t port_number;
	std::unique_ptr<AgentServer> rpc_server;

	std::unique_ptr<bu::Logger> logger;
	std::vector<std::shared_ptr<AgentClient>> rpc_clients;

	// Plugin required
	static bool configured;

	static bool Configure(PF_ObjectParams * params);


	int GetSystemId(std::string const & path) const;

	std::string GetNetAddress(int system_id) const;

	std::shared_ptr<AgentClient> GetAgentClient(int system_id);

};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_AGENT_PROXY_GRPC_H_
