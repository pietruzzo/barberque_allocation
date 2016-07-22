#include "agent_impl.h"

namespace bbque
{
namespace plugins
{

grpc::Status AgentImpl::GetResourceStatus(
        grpc::ServerContext * context,
        const bbque::ResourceStatusRequest * request,
        bbque::ResourceStatusReply * reply)
{

	std::cerr << "[DBG] === GetResourceStatus ===" << std::endl;
	if (request->path().empty()) {
		std::cerr << "[ERR] Invalid resource path specified" << std::endl;
	}

	// Call ResourceAccounter member functions...
	int64_t total = 100, used = 20;
	reply->set_total(total);
	reply->set_used(used);

	// Power information...
	uint32_t degr_perc = 1, power_mw = 35000, temp = 40;
	reply->set_degradation(degr_perc);
	reply->set_power_mw(power_mw);
	reply->set_temperature(temp);

	return grpc::Status::OK;
}

grpc::Status AgentImpl::SetNodeManagementAction(
        grpc::ServerContext * context,
        const bbque::NodeManagementAction * action,
        bbque::ErrorCode * error)
{

	std::cerr << "[DBG] === SetNodeManagementAction ===" << std::endl;
	std::cout << "[INF] Management action: " << action->value() << std::endl;
	error->set_value(bbque::ErrorCode::OK);

	return grpc::Status::OK;
}

} // namespace plugins

} // namespace bbque
