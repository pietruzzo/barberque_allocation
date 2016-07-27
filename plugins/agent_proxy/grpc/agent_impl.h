#ifndef BBQUE_AGENT_PROXY_GRPC_IMPL_H_
#define BBQUE_AGENT_PROXY_GRPC_IMPL_H_

#include <grpc/grpc.h>
#include "agent_com.grpc.pb.h"

namespace bbque
{
namespace plugins
{

class AgentImpl final: public bbque::RemoteAgent::Service
{

public:
	explicit AgentImpl() {}
	virtual ~AgentImpl() {}

	grpc::Status GetResourceStatus(
	        grpc::ServerContext * context,
	        const bbque::ResourceStatusRequest * request,
	        bbque::ResourceStatusReply * reply) override;

	grpc::Status SetNodeManagementAction(
	        grpc::ServerContext * context,
	        const bbque::NodeManagementRequest * action,
	        bbque::GenericReply * error) override;
};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_AGENT_PROXY_GRPC_IMPL_H_
