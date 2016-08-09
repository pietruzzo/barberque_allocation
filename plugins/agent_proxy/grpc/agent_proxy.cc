#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include <boost/program_options/options_description.hpp>

#include "bbque/config.h"
#include "bbque/pp/platform_description.h"
#include "bbque/res/resource_utils.h"
#include "bbque/res/resource_type.h"

#include "agent_proxy.h"

namespace bbque
{
namespace plugins
{

using bbque::agent::ExitCode_t;

boost::program_options::variables_map agent_proxy_opts_value;

uint32_t AgentProxyGRPC::port_num = BBQUE_AGENT_PROXY_PORT_DEFAULT;

// =======================[ Static plugin interface ]=========================

bool AgentProxyGRPC::configured = false;

void * AgentProxyGRPC::Create(PF_ObjectParams * params) {
	if (!Configure(params))
		return nullptr;

	return new AgentProxyGRPC();
}

int32_t AgentProxyGRPC::Destroy(void * plugin) {
	if (!plugin)
		return -1;
	delete (AgentProxyGRPC *)plugin;
	return 0;
}

bool AgentProxyGRPC::Configure(PF_ObjectParams * params) {
	if (configured)
		return true;

	// Declare the supported options
	boost::program_options::options_description
		agent_proxy_opts_desc("AgentProxy options");
	agent_proxy_opts_desc.add_options()
		(MODULE_CONFIG".port", boost::program_options::value<uint32_t>
		 (&port_num)->default_value(BBQUE_AGENT_PROXY_PORT_DEFAULT),
		 "Server port number");

	// Get configuration params
	PF_Service_ConfDataIn data_in;
	data_in.opts_desc = &agent_proxy_opts_desc;
	PF_Service_ConfDataOut data_out;
	data_out.opts_value = &agent_proxy_opts_value;

	PF_ServiceData sd;
	sd.id = MODULE_NAMESPACE;
	sd.request  = &data_in;
	sd.response = &data_out;

	int32_t response =
	        params->platform_services->InvokeService(PF_SERVICE_CONF_DATA, sd);

	if (response != PF_SERVICE_DONE)
		return false;

	return true;
}

// =============================================================================

AgentProxyGRPC::AgentProxyGRPC() {
	logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	server_address_port = std::string("0.0.0.0:") + std::to_string(port_num);
	logger->Info("AgentProxy Server will listen on %s",
		server_address_port.c_str());
	Setup("AgentProxyServer", MODULE_NAMESPACE".srv");
}

AgentProxyGRPC::~AgentProxyGRPC() {
	logger->Info("Destroying the AgentProxy module...");
	clients.clear();
	systems.clear();
}

void AgentProxyGRPC::SetPlatformDescription(
		bbque::pp::PlatformDescription const * platform) {

	if ((platform == nullptr) || (platform->GetSystemsAll().empty())) {
		logger->Error("No system descriptors");
		return;
	}

	systems = platform->GetSystemsAll();
	logger->Debug("Systems in the managed platform: %d", systems.size());

	local_sys_id = platform->GetLocalSystem().GetId();
	logger->Debug("Local system id: %d", local_sys_id);
}

void AgentProxyGRPC::StartServer() {
	if (server != nullptr) {
		logger->Warn("Server already started");
		return;
	}
	logger->Info("Starting the server task...");
	Start();
}

void AgentProxyGRPC::Task() {
	logger->Debug("Server task launched");
	RunServer();
	logger->Info("Server stopped");
	Notify();
}

void AgentProxyGRPC::RunServer() {
	grpc::ServerBuilder builder;
	builder.AddListeningPort(
	        server_address_port, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	server = builder.BuildAndStart();
	logger->Notice("Server listening on %s", server_address_port.c_str());
	server->Wait();
}

void AgentProxyGRPC::StopServer() {
	logger->Info("Stopping the server task...");
	if (server == nullptr) {
		logger->Warn("Server already stopped");
		return;
	}
	server->Shutdown();
}

void AgentProxyGRPC::WaitForServerToStop() {
	Wait();
}


uint16_t AgentProxyGRPC::GetSystemId(const std::string & system_path) const {
	return bbque::res::ResourcePathUtils::GetID(
		system_path, bbque::res::GetResourceTypeString(
			bbque::res::ResourceType::SYSTEM));
}

std::string AgentProxyGRPC::GetNetAddress(int system_id) const {
	(void) system_id;
	// Extract the sysID from the resource path.
	return "0.0.0.0";
}

std::shared_ptr<AgentClient> AgentProxyGRPC::GetAgentClient(uint16_t system_id) {
	logger->Debug("Retrieving a client for system %d", system_id);
	assert(!systems.empty());

	if (system_id >= systems.size()) {
		logger->Error("System %d not registered", system_id);
		return nullptr;
	}

	if(clients.size() <= system_id) {
		logger->Debug("Creating a client for system %d", system_id);
		std::string server_address_port(
			systems.at(system_id).GetNetAddress());
		server_address_port.append(":" + std::to_string(port_num));
		logger->Debug("Allocating a client to connect %s",
			server_address_port.c_str());

		std::shared_ptr<AgentClient> client =
		        std::make_shared<AgentClient>(
				local_sys_id, server_address_port);
		clients.push_back(client);
	}
	logger->Debug("Client instances: %d", clients.size());
	return clients.at(system_id);
}


ExitCode_t AgentProxyGRPC::GetResourceStatus(
		std::string const & resource_path,
		agent::ResourceStatus & status) {
	uint16_t system_id = GetSystemId(resource_path);
	std::shared_ptr<AgentClient> client(GetAgentClient(system_id));
	logger->Debug("Vector size: %d", clients.size());

	if(client == nullptr) {
		logger->Error("Client for <%s> not ready", resource_path.c_str());
		return agent::ExitCode_t::AGENT_UNREACHABLE;
	}
	return client->GetResourceStatus(resource_path, status);
//    rpc_clients[0]->GetResourceStatus(status);
}


ExitCode_t AgentProxyGRPC::GetWorkloadStatus(
		std::string const & path,
		agent::WorkloadStatus & status) {
	return agent::ExitCode_t::AGENT_UNREACHABLE;
}

ExitCode_t AgentProxyGRPC::GetWorkloadStatus(
		int system_id,
		agent::WorkloadStatus & status) {
	return agent::ExitCode_t::AGENT_UNREACHABLE;

}


ExitCode_t AgentProxyGRPC::GetChannelStatus(
		std::string const & path,
		agent::ChannelStatus & status) {
	return agent::ExitCode_t::AGENT_UNREACHABLE;
}

ExitCode_t AgentProxyGRPC::GetChannelStatus(
		int system_id,
		agent::ChannelStatus & status) {
	return agent::ExitCode_t::AGENT_UNREACHABLE;

}


// ------------- Multi-agent management functions ------------------

ExitCode_t AgentProxyGRPC::SendJoinRequest(std::string const & path) {

	return agent::ExitCode_t::AGENT_UNREACHABLE;
}

ExitCode_t AgentProxyGRPC::SendJoinRequest(int system_id) {

	return agent::ExitCode_t::AGENT_UNREACHABLE;
}


ExitCode_t AgentProxyGRPC::SendDisjoinRequest(std::string const & path) {

	return agent::ExitCode_t::AGENT_UNREACHABLE;
}

ExitCode_t AgentProxyGRPC::SendDisjoinRequest(int system_id) {

	return agent::ExitCode_t::AGENT_UNREACHABLE;
}


// ----------- Scheduling / Resource allocation functions ----------

ExitCode_t AgentProxyGRPC::SendScheduleRequest(
		std::string const & path,
		agent::ApplicationScheduleRequest const & request) {

	return agent::ExitCode_t::AGENT_UNREACHABLE;
}

} // namespace plugins

} // namespace bbque
