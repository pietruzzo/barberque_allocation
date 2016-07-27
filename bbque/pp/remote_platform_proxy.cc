
#include "bbque/modules_factory.h"
#include "bbque/pp/remote_platform_proxy.h"
#include "bbque/config.h"

namespace bbque {
namespace pp {

RemotePlatformProxy::RemotePlatformProxy() {
	logger = bu::Logger::GetLogger(REMOTE_PLATFORM_PROXY_NAMESPACE);
	assert(logger);
}

const char* RemotePlatformProxy::GetPlatformID(int16_t system_id) const {
	(void) system_id;
	logger->Error("GetPlatformID - Not implemented.");
	return "";
}

const char* RemotePlatformProxy::GetHardwareID(int16_t system_id) const {
	(void) system_id;
	logger->Error("GetHardwareID - Not implemented.");
	return "";
}


RemotePlatformProxy::ExitCode_t RemotePlatformProxy::Setup(AppPtr_t papp) {
	(void) papp;
	logger->Error("Setup - Not implemented.");
	return PLATFORM_OK;
}

RemotePlatformProxy::ExitCode_t RemotePlatformProxy::LoadPlatformData() {

	ExitCode_t ec = LoadAgentProxy();
	if (ec != PLATFORM_OK) {
		logger->Error("Cannot start Agent Proxy");
		return ec;
	}

	return PLATFORM_OK;
}

RemotePlatformProxy::ExitCode_t RemotePlatformProxy::LoadAgentProxy() {
	agent_proxy = std::unique_ptr<bbque::plugins::AgentProxyIF>(
		ModulesFactory::GetModule<bbque::plugins::AgentProxyIF>(
			std::string(AGENT_PROXY_NAMESPACE) + ".grpc"));

	if (agent_proxy == nullptr) {
		logger->Fatal("Agent Proxy plugin loading failed!");
		return PLATFORM_AGENT_PROXY_ERROR;
	}
	logger->Info("Agent Proxy plugin ready");

	return PLATFORM_OK;
}

RemotePlatformProxy::ExitCode_t RemotePlatformProxy::Refresh() {
	logger->Error("Refresh - Not implemented.");
	return PLATFORM_OK;
}

RemotePlatformProxy::ExitCode_t RemotePlatformProxy::Release(AppPtr_t papp) {
	(void) papp;
	logger->Error("Release - Not implemented.");
	return PLATFORM_OK;
}

RemotePlatformProxy::ExitCode_t RemotePlatformProxy::ReclaimResources(AppPtr_t papp) {
	(void) papp;
	logger->Error("ReclaimResources - Not implemented.");
	return PLATFORM_OK;
}

RemotePlatformProxy::ExitCode_t RemotePlatformProxy::MapResources(
		AppPtr_t papp,
		ResourceAssignmentMapPtr_t pres,
                bool excl) {
	(void) papp;
	(void) pres;
	(void) excl;

	logger->Error("MapResources - Not implemented.");
	return PLATFORM_OK;
}

} // namespace pp
} // namespace bbque

