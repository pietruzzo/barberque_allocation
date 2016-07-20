#ifndef BBQUE_REMOTE_PLATFORM_PROXY_H
#define BBQUE_REMOTE_PLATFORM_PROXY_H


#include "bbque/platform_proxy.h"
#include "bbque/pp/distributed_proxy.h"

#define REMOTE_PLATFORM_PROXY_NAMESPACE "bb.pp.rpp"

namespace bbque {
namespace pp {
class RemotePlatformProxy : public PlatformProxy, public DistributedProxy
{
public:

	RemotePlatformProxy();

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
	 * @brief The logger used by the worker thread
	 */
	std::unique_ptr<bu::Logger> logger;

};
}   // namespace pp
}   // namespace bbque

#endif // BBQUE_REMOTE_PLATFORM_PROXY_H
