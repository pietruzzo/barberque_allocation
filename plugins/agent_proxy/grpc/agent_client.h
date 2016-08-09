#ifndef BBQUE_AGENT_PROXY_GRPC_CLIENT_H_
#define BBQUE_AGENT_PROXY_GRPC_CLIENT_H_

#include <memory>
#include <string>

#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include <grpc++/support/time.h>

#include "bbque/plugins/agent_proxy_types.h"
#include "agent_com.grpc.pb.h"

namespace bbque
{
namespace plugins
{

using bbque::agent::ExitCode_t;

class AgentClient
{

public:

	AgentClient(int local_sys_id, const std::string & _address_port);

	bool IsConnected();

	// ---------- Status

	ExitCode_t GetResourceStatus(
	        const std::string & resource_path,
	        agent::ResourceStatus & resource_status);

	ExitCode_t GetWorkloadStatus(agent::WorkloadStatus & workload_status);

	ExitCode_t GetChannelStatus(agent::ChannelStatus & channel_status);

	// ----------- Multi-agent management

	ExitCode_t SendJoinRequest();

	ExitCode_t SendDisjoinRequest();

	// ----------- Scheduling / Resource allocation

	ExitCode_t SendScheduleRequest(
	        agent::ApplicationScheduleRequest const & request);

private:

	int local_system_id;

	std::string server_address_port;

	std::shared_ptr<grpc::Channel> channel;

	std::unique_ptr<bbque::RemoteAgent::Stub> service_stub;

	ExitCode_t Connect();
};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_AGENT_PROXY_GRPC_CLIENT_H_
