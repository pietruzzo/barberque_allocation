#ifndef BBQUE_AGENT_PROXY_GRPC_IMPL_H_
#define BBQUE_AGENT_PROXY_GRPC_IMPL_H_

#include "bbque/plugins/agent_proxy_if.h"
#include "bbque/system.h"
#include "bbque/utils/logging/logger.h"

#include <grpc/grpc.h>
#include "agent_com.grpc.pb.h"

namespace bbque
{
namespace plugins
{

class AgentImpl final: public bbque::RemoteAgent::Service
{

public:
	explicit AgentImpl():
		system(bbque::System::GetInstance()),
		logger(bbque::utils::Logger::GetLogger(AGENT_PROXY_NAMESPACE".grpc.imp")) {
	}

	virtual ~AgentImpl() {}

	grpc::Status GetResourceStatus(
	        grpc::ServerContext * context,
	        const bbque::ResourceStatusRequest * request,
	        bbque::ResourceStatusReply * reply) override;

	grpc::Status GetWorkloadStatus(
		grpc::ServerContext * context,
		const bbque::GenericRequest * request,
		bbque::WorkloadStatusReply * reply) override;

	grpc::Status GetChannelStatus(
		grpc::ServerContext * context,
		const bbque::GenericRequest * request,
		bbque::ChannelStatusReply * reply) override;

	grpc::Status SetNodeManagementAction(
	        grpc::ServerContext * context,
	        const bbque::NodeManagementRequest * action,
	        bbque::GenericReply * error) override;
private:

	bbque::System & system;

	std::unique_ptr<bbque::utils::Logger> logger;

};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_AGENT_PROXY_GRPC_IMPL_H_
