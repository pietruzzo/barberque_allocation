#include "agent_client.h"

namespace bbque
{
namespace plugins
{

using bbque::agent::ExitCode_t;

AgentClient::AgentClient(const std::string & _address_port):
	server_address_port(_address_port)
{
	Connect();
}

ExitCode_t AgentClient::Connect()
{
	std::cout << "[INF] Connecting to " << server_address_port << std::endl;
	if (channel == nullptr) {
		channel = grpc::CreateChannel(
		                  server_address_port, grpc::InsecureChannelCredentials());
		std::cout << "[INF] Channel open" << std::endl;
	} else {
		std::cout << "[INF] Channel already open" << std::endl;
	}
	// Get the stub
	service_stub = bbque::RemoteAgent::NewStub(channel);
	if (service_stub == nullptr) {
		std::cerr << "[ERR] Channel opening failed" << std::endl;
		return agent::ExitCode_t::AGENT_UNREACHABLE;
	}
	std::cout << "[INF] Stub ready" << std::endl;

	return ExitCode_t::OK;
}


bool AgentClient::IsConnected()
{
	if (!channel || !service_stub) {
		return false;
	}
	if ((channel->GetState(false) != 0) && (channel->GetState(false) != 2))
		return true;
	return true;
}

// ---------- Status

ExitCode_t AgentClient::GetResourceStatus(
        std::string const & resource_path,
        agent::ResourceStatus & resource_status)
{
	bbque::ResourceStatusRequest request;
	bbque::ResourceStatusReply reply;
	grpc::ClientContext context;
	grpc::Status status;

	// Connect...
	ExitCode_t exit_code = Connect();
	if (exit_code != ExitCode_t::OK) {
		std::cerr << "[ERR] Connection failed" << std::endl;
		return exit_code;
	}
	/*
	        // Test the timeout
	        std::chrono::system_clock::time_point deadline =
	            std::chrono::system_clock::now() + std::chrono::seconds(1);
	        context.set_deadline(deadline);
	*/
	// Do RPC call
	request.set_path(resource_path);
	request.set_average(false);
	status = service_stub->GetResourceStatus(&context, request, &reply);
	if (!status.ok()) {
		std::cerr << "[ERR] " << status.error_message() << std::endl;
		return ExitCode_t::AGENT_DISCONNECTED;
	}

	resource_status.total = reply.total();
	resource_status.used  = reply.used();
	resource_status.power_mw    = reply.power_mw();
	resource_status.temperature = reply.temperature();
	resource_status.degradation = reply.degradation();

	return ExitCode_t::OK;
}

ExitCode_t AgentClient::GetWorkloadStatus(
        agent::WorkloadStatus & workload_status)
{

	return ExitCode_t::OK;
}

ExitCode_t AgentClient::GetChannelStatus(
        agent::ChannelStatus & channel_status)
{

	return ExitCode_t::OK;
}

// ----------- Multi-agent management

ExitCode_t AgentClient::SendJoinRequest()
{

	return ExitCode_t::OK;
}

ExitCode_t AgentClient::SendDisjoinRequest()
{

	return ExitCode_t::OK;
}

// ----------- Scheduling / Resource allocation

ExitCode_t AgentClient::SendScheduleRequest(
        agent::ApplicationScheduleRequest const & request)
{

	return ExitCode_t::OK;
}

} // namespace plugins

} // namespace bbque

