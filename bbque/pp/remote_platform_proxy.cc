#include "bbque/pp/remote_platform_proxy.h"
#include "bbque/config.h"

namespace bbque {
namespace pp {

RemotePlatformProxy::RemotePlatformProxy()
{
	// Get a logger module
	logger = bu::Logger::GetLogger(REMOTE_PLATFORM_PROXY_NAMESPACE);
	assert(logger);

}


/**
 * @brief Return the Platform specific string identifier
 */
const char* RemotePlatformProxy::GetPlatformID(int16_t system_id) const {
	(void) system_id;
	logger->Error("GetPlatformID - Not implemented.");
	return "";
}

/**
 * @brief Return the Hardware identifier string
 */
const char* RemotePlatformProxy::GetHardwareID(int16_t system_id) const {
	(void) system_id;
	logger->Error("GetHardwareID - Not implemented.");
	return "";
}
/**
 * @brief Platform specific resource setup interface.
 */
RemotePlatformProxy::ExitCode_t RemotePlatformProxy::Setup(AppPtr_t papp) {
	(void) papp;
	logger->Error("Setup - Not implemented.");
	return PLATFORM_OK;
}


/**
 * @brief Platform specific resources enumeration
 *
 * The default implementation of this method loads the TPD, is such a
 * function has been enabled
 */
RemotePlatformProxy::ExitCode_t RemotePlatformProxy::LoadPlatformData() {
	logger->Error("LoadPlatformData - Not implemented.");
	return PLATFORM_OK;
}



/**
 * @brief Platform specific resources refresh
 */
RemotePlatformProxy::ExitCode_t RemotePlatformProxy::Refresh() {
	logger->Error("Refresh - Not implemented.");
	return PLATFORM_OK;
}

/**
 * @brief Platform specific resources release interface.
 */
RemotePlatformProxy::ExitCode_t RemotePlatformProxy::Release(AppPtr_t papp) {
	(void) papp;
	logger->Error("Release - Not implemented.");
	return PLATFORM_OK;
}

/**
 * @brief Platform specific resource claiming interface.
 */
RemotePlatformProxy::ExitCode_t RemotePlatformProxy::ReclaimResources(AppPtr_t papp) {
	(void) papp;
	logger->Error("ReclaimResources - Not implemented.");
	return PLATFORM_OK;
}

/**
 * @brief Platform specific resource binding interface.
 */
RemotePlatformProxy::ExitCode_t RemotePlatformProxy::MapResources(AppPtr_t papp, UsagesMapPtr_t pres,
                bool excl) {
	(void) papp;
	(void) pres;
	(void) excl;

	logger->Error("MapResources - Not implemented.");
	return PLATFORM_OK;
}

} // namespace pp
} // namespace bbque

