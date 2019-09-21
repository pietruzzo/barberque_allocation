/*
 * Copyright (C) 2016  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

#include "bbque/plugins/agent_proxy_if.h"
#include "bbque/utils/logging/logger.h"
#include "bbque/utils/timer.h"
#include "agent_com.grpc.pb.h"

namespace bbque
{
namespace plugins
{

using bbque::agent::ExitCode_t;

class AgentClient
{

public:

	AgentClient(int _local_id, int _remote_id, const std::string & _address_port);

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

	ExitCode_t ResourceRequest(
	        agent::ResourceAllocation & resource_request,
		agent::ResourceAllocation & resource_reply);

private:

	int local_system_id;

	int remote_system_id;

	std::string remote_address_port;

	std::shared_ptr<grpc::Channel> channel;

	std::unique_ptr<bbque::RemoteAgent::Stub> service_stub;

	std::unique_ptr<bbque::utils::Logger> logger;

	bbque::utils::Timer timer;


	ExitCode_t Connect();
};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_AGENT_PROXY_GRPC_CLIENT_H_
