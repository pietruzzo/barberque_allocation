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

	logger->Debug("=== GetResourceStatus ===");
	if (request->path().empty()) {
		logger->Error("Invalid resource path specified");
		return grpc::Status::CANCELLED;
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
        const bbque::NodeManagementRequest * action,
        bbque::GenericReply * error)
{

	logger->Debug(" === SetNodeManagementAction ===");
	logger->Info("Management action: %d ", action->value());
	error->set_value(bbque::GenericReply::OK);

	return grpc::Status::OK;
}

} // namespace plugins

} // namespace bbque
