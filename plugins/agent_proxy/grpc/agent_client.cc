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

#include "agent_client.h"

namespace bbque
{
namespace plugins
{

using bbque::agent::ExitCode_t;

AgentClient::AgentClient(int _local_id, int _remote_id, const std::string & _address_port):
	local_system_id(_local_id),
	remote_system_id(_remote_id),
	remote_address_port(_address_port)
{
	logger = bbque::utils::Logger::GetLogger(AGENT_PROXY_NAMESPACE".grpc.cln");
	Connect();
}

ExitCode_t AgentClient::Connect()
{
	logger->Debug("Connecting to %s...", remote_address_port.c_str());
	if (channel != nullptr) {
		logger->Debug("Channel already open");
		return agent::ExitCode_t::OK;
	}

	channel = grpc::CreateChannel(
			  remote_address_port, grpc::InsecureChannelCredentials());
	logger->Debug("Channel open");

	service_stub = bbque::RemoteAgent::NewStub(channel);
	if (service_stub == nullptr) {
		logger->Error("Channel opening failed");
		return agent::ExitCode_t::AGENT_UNREACHABLE;
	}
	logger->Debug("Stub ready");

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
		agent::ResourceStatus & resource_status) {
	// Connect...
	ExitCode_t exit_code = Connect();
	if (exit_code != ExitCode_t::OK) {
		logger->Error("ResourceStatus: Connection failed");
		return exit_code;
	}

	// Do RPC call
	bbque::ResourceStatusRequest request;
	request.set_sender_id(local_system_id);
	request.set_dest_id(remote_system_id);
	request.set_path(resource_path);
	request.set_average(false);

	grpc::Status status;
	grpc::ClientContext context;
	bbque::ResourceStatusReply reply;

	logger->Debug("ResourceStatus: Calling implementation...");
	status = service_stub->GetResourceStatus(&context, request, &reply);
	if (!status.ok()) {
		logger->Error("ResourceStatus: Returned code %d", status.error_code());
		return ExitCode_t::AGENT_DISCONNECTED;
	}

	resource_status.total = reply.total();
	resource_status.used  = reply.used();
	resource_status.power_mw    = reply.power_mw();
	resource_status.temperature = reply.temperature();
	resource_status.degradation = reply.degradation();
        logger->Debug("ResourceStatus: T:%3d, U:%3d, PWR:%3d, TEMP:%5d",
		resource_status.total, resource_status.used,
		resource_status.power_mw, resource_status.temperature);

	return ExitCode_t::OK;
}

ExitCode_t AgentClient::GetWorkloadStatus(
		agent::WorkloadStatus & workload_status) {

	ExitCode_t exit_code = Connect();
	if (exit_code != ExitCode_t::OK) {
		logger->Error("WorkloadStatus: Connection failed");
		return exit_code;
	}

	bbque::GenericRequest request;
	request.set_sender_id(local_system_id);
	request.set_dest_id(remote_system_id);
	grpc::Status status;
	grpc::ClientContext context;
	bbque::WorkloadStatusReply reply;

	logger->Debug("WorkloadStatus: Calling implementation...");
	status = service_stub->GetWorkloadStatus(&context, request, &reply);
	if (!status.ok()) {
		logger->Error("WorkloadStatus: Returned code %d", status.error_code());
		return ExitCode_t::AGENT_DISCONNECTED;
	}

	workload_status.nr_ready   = reply.nr_ready();
	workload_status.nr_running = reply.nr_running();
	logger->Debug("WorkloadStatus: RUN: %2d, RDY: %2d",
		workload_status.nr_ready, workload_status.nr_running);

	return ExitCode_t::OK;
}

ExitCode_t AgentClient::GetChannelStatus(agent::ChannelStatus & channel_status) {
	timer.start();
	ExitCode_t exit_code = Connect();
	if (exit_code != ExitCode_t::OK) {
		logger->Error("ChannelStatus: Connection failed");
		timer.stop();
		return exit_code;
	}

	bbque::GenericRequest request;
	request.set_sender_id(local_system_id);
	request.set_dest_id(remote_system_id);
	grpc::Status status;
	grpc::ClientContext context;
	bbque::ChannelStatusReply reply;

	logger->Debug("ChannelStatus: Calling implementation...");
	status = service_stub->GetChannelStatus(&context, request, &reply);
	if (!status.ok()) {
		logger->Error("ChannelStatus: Returned code %d", status.error_code());
		timer.stop();
		return ExitCode_t::AGENT_DISCONNECTED;
	}

	channel_status.connected = reply.connected();
	timer.stop();
	channel_status.latency_ms = timer.getElapsedTimeMs();
	logger->Debug("ChannelStatus: Connected: %d, Latency: %.0f ms",
		channel_status.connected, channel_status.latency_ms);

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

ExitCode_t AgentClient::ResourceRequest(
	agent::ResourceAllocation & resource_request,
	agent::ResourceAllocation & resource_reply)
{
    ExitCode_t exit_code = Connect();
	if (exit_code != ExitCode_t::OK) {
		logger->Error("ResourceRequest: Connection failed");
		return exit_code;
	}

	bbque::ResourceAlloc request;
	grpc::Status status;
	grpc::ClientContext context;
	bbque::ResourceAlloc reply;

    request.set_sender_id(local_system_id);
    request.set_dest_id(remote_system_id);
    request.set_trans_id(resource_request.trans_id);
    request.set_memory(resource_request.memory);

    for(const agent::ResourceAllocation::ProcessingUnitsRequest & element : resource_request.proc){
	bbque::CuU * cuu = request.add_proc();
        cuu->set_arch(element.arch);
	cuu->set_num(element.num);
    }
	logger->Debug("ResourceRequest: Calling implementation...");
	status = service_stub->ResourceRequest(&context, request, &reply);
	if (!status.ok()) {
		logger->Error("ResourceRequest: Returned code %d", status.error_code());
		return ExitCode_t::AGENT_DISCONNECTED;
	}

	//Return values
	resource_reply.trans_id = reply.trans_id();
    resource_reply.memory = reply.memory();
    logger->Debug("ResourceRequest: Reading reapeated messages");
    for (const bbque::CuU & cuu : reply.proc()) {
        resource_reply.proc.push_back({cuu.arch(), cuu.num()});
    }

	return ExitCode_t::OK;
}

ExitCode_t AgentClient::Agreement(
	agent::ResourceAllocation & resource_request,
	agent::AgreementReply & resource_reply)
{
    bbque::ResourceAlloc request;
	grpc::Status status;
	grpc::ClientContext context;
	bbque::ResourceAlloc reply;

    request.set_sender_id(local_system_id);
    request.set_dest_id(remote_system_id);
    request.set_trans_id(resource_request.trans_id);
    request.set_memory(resource_request.memory);

    for(const agent::ResourceAllocation::ProcessingUnitsRequest & element : resource_request.proc){
        bbque::CuU * cuu = request.add_proc();
        cuu->set_arch(element.arch);
        cuu->set_num(element.num);
    }

	logger->Debug("Agreement: Calling implementation...");
	status = service_stub->Agreement(&context, request, &reply);
	if (!status.ok()) {
		logger->Error("Agreement: Returned code %d", status.error_code());
		return ExitCode_t::AGENT_DISCONNECTED;
	}

	logger->Debug("Agreement: Reading reapeated messages");
    for (const bbque::CuU & cuu : reply.proc()) {
        for (const std::string & path : cuu.path()) {
            resource_reply.proc_paths.push_back({cuu.arch(), path});
        }
    }

	return ExitCode_t::OK;
}

} // namespace plugins

} // namespace bbque

