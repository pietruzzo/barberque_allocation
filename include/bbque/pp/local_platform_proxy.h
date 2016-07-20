#ifndef BBQUE_LOCAL_PLATFORM_PROXY_H_
#define BBQUE_LOCAL_PLATFORM_PROXY_H_

#include "bbque/platform_proxy.h"

#include <memory>
#include <vector>


namespace bbque {
namespace pp {
class LocalPlatformProxy : public PlatformProxy
{
public:

	LocalPlatformProxy();

	/**
	 * @brief Return the Platform specific string identifier
	 */
	virtual const char* GetPlatformID(int16_t system_id=-1) const;

	/**
	 * @brief Return the Hardware identifier string
	 */
	virtual const char* GetHardwareID(int16_t system_id=-1) const;
	/**
	 * @brief Platform specific resource setup interface.
	 */
	virtual ExitCode_t Setup(AppPtr_t papp);

	/**
	 * @brief Platform specific resources enumeration
	 *
	 * The default implementation of this method loads the TPD, is such a
	 * function has been enabled
	 */
	virtual ExitCode_t LoadPlatformData();

	/**
	 * @brief Platform specific resources refresh
	 */
	virtual ExitCode_t Refresh();

	/**
	 * @brief Platform specific resources release interface.
	 */
	virtual ExitCode_t Release(AppPtr_t papp);

	/**
	 * @brief Platform specific resource claiming interface.
	 */
	virtual ExitCode_t ReclaimResources(AppPtr_t papp);

	/**
	 * @brief Platform specific resource binding interface.
	 */
	virtual ExitCode_t MapResources(AppPtr_t papp, ResourceAssignmentMapPtr_t pres,
	                                bool excl = true) ;

private:
	/**
	 * @brief The host platform proxy, e.g. linux or android
	 */
	std::unique_ptr<PlatformProxy> host;

	/**
	 * @brief The list of auxiliary platform proxy, like OpenCL, Adapteva,
	 *        Process Listener, etc.
	 */
	std::vector<std::unique_ptr<PlatformProxy>> aux;

};

}   // namespace pp
}   // namespace bbque

#endif // LOCALPLATFORMPROXY_H
