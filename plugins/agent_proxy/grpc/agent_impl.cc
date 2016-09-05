#include "agent_impl.h"

#include "bbque/config.h"

#ifdef CONFIG_BBQUE_PM
  #include "bbque/pm/power_manager.h"
#endif

namespace bbque
{
namespace plugins
{

grpc::Status AgentImpl::GetResourceStatus(
		grpc::ServerContext * context,
		const bbque::ResourceStatusRequest * request,
		bbque::ResourceStatusReply * reply) {

	logger->Debug("=== GetResourceStatus ===");
	if (request->path().empty()) {
		logger->Error("Invalid resource path specified");
		return grpc::Status::CANCELLED;
	}

	// Call ResourceAccounter member functions...
	int64_t total = system.ResourceTotal(request->path());
	int64_t used  = system.ResourceUsed(request->path());
	reply->set_total(total);
	reply->set_used(used);

	// Power information...
	bbque::res::ResourcePtr_t resource(system.GetResource(request->path()));
	if (resource == nullptr) {
		logger->Error("Invalid resource path specified");
		return grpc::Status::CANCELLED;
	}

	bbque::res::ResourcePathPtr_t resource_path(
		system.GetResourcePath(request->path()));
	if (resource_path == nullptr) {
		logger->Error("Invalid resource path specified");
		return grpc::Status::CANCELLED;
	}

	uint32_t degr_perc = 100;
	uint32_t power_mw = 0, temp = 0;
#ifdef CONFIG_BBQUE_PM
	bbque::PowerManager & pm(bbque::PowerManager::GetInstance());
	pm.GetPowerUsage(resource_path, power_mw);
	pm.GetTemperature(resource_path, temp);
#endif
	reply->set_degradation(degr_perc);
	reply->set_power_mw(power_mw);
	reply->set_temperature(temp);

	return grpc::Status::OK;
}


grpc::Status AgentImpl::GetWorkloadStatus(
		grpc::ServerContext * context,
		const bbque::GenericRequest * request,
		bbque::WorkloadStatusReply * reply) {

	logger->Debug("WorkloadStatus: Request from system %d", request->sender_id());
	reply->set_nr_running(system.ApplicationsCount(
		bbque::app::ApplicationStatusIF::RUNNING));
	reply->set_nr_ready(system.ApplicationsCount(
		bbque::app::ApplicationStatusIF::READY));

	return grpc::Status::OK;
}

	return grpc::Status::OK;
}


grpc::Status AgentImpl::SetNodeManagementAction(
		grpc::ServerContext * context,
		const bbque::NodeManagementRequest * action,
		bbque::GenericReply * error) {

	logger->Debug(" === SetNodeManagementAction ===");
	logger->Info("Management action: %d ", action->value());
	error->set_value(bbque::GenericReply::OK);

	return grpc::Status::OK;
}

} // namespace plugins

} // namespace bbque
