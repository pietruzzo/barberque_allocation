#ifndef BBQUE_PLATFORM_MANAGER_H
#define BBQUE_PLATFORM_MANAGER_H

#include "bbque/pp/local_platform_proxy.h"
#include "bbque/pp/remote_platform_proxy.h"
#include "bbque/utils/worker.h"

#include <memory>

#define PLATFORM_MANAGER_NAMESPACE "bq.plm"

#define PLATFORM_MANAGER_EV_REFRESH  0
#define PLATFORM_MANAGER_EV_COUNT    1

namespace bbque {

class PlatformManager : public PlatformProxy, public utils::Worker, public CommandHandler
{
public:
	/**
	 * @brief Get a reference to the paltform proxy
	 */
	static PlatformManager & GetInstance();

	/**
	 * @brief Return the Platform specific string identifier
	 * @param system_id It specifies from which system take the
	 *       platform identifier. If not specified or equal
	 *       to "-1", the platorm id of the local system is returned.
	 */
	virtual const char* GetPlatformID(int16_t system_id=-1) const override;

	/**
	 * @brief Return the Hardware identifier string
	 * @param system_id It specifies from which system take the
	 *       platform idenfier. If not specified or equal
	 *       to "-1", the hw id of the local system is returned.
	 */
	virtual const char* GetHardwareID(int16_t system_id=-1) const override;

	/**
	 * @brief Platform specific resource setup interface.
	 * @warning Not implemented in PlatformManager!
	 */
	virtual ExitCode_t Setup(SchedPtr_t papp) override;

	/**
	 * @brief Platform specific resources enumeration
	 *
	 * The default implementation of this method loads the TPD, is such a
	 * function has been enabled
	 */
	virtual ExitCode_t LoadPlatformData() override;

	/**
	 * @brief Platform specific resources refresh
	 */
	virtual ExitCode_t Refresh() override;

	/**
	 * @brief Platform specific resources release interface.
	 */
	virtual ExitCode_t Release(SchedPtr_t papp) override;

	/**
	 * @brief Platform specific resource claiming interface.
	 */
	virtual ExitCode_t ReclaimResources(SchedPtr_t papp) override;

	/**
	 * @brief Platform specific termiantion.
	 */
	virtual void Exit() override;

	/**
	 * @brief Bind the specified resources to the specified application.
	 *
	 * @param papp The application which resources are assigned
	 * @param pres The resources to be assigned
	 * @param excl If true the specified resources are assigned for exclusive
	 * usage to the application
	 */
	virtual ExitCode_t MapResources(
		SchedPtr_t papp, ResourceAssignmentMapPtr_t pres, bool excl = true) override;

	/**
	 * @brief Check if the resource is a "high-performance" is single-ISA
	 * heterogeneous platforms
	 *
	 * @return true if so, false otherwise
	 */
	virtual bool IsHighPerformance(bbque::res::ResourcePathPtr_t const & path) const;

	/**
	 * @brief Load the configuration via the corresponding plugin
	 *        It encapsulate exceptions coming from plugins.
	 * @return PL_SUCCESS in case of successful load, an error
	 *         otherwise.
	 */
	ExitCode_t LoadPlatformConfig();

	/**
	 * @brief Get a reference to the local platform proxy
	 * @return A PlatformProxy reference
	 */
	inline PlatformProxy const & GetLocalPlatformProxy() {
		return *lpp;
	}

#ifdef CONFIG_BBQUE_DIST_MODE
	/**
	 * @brief Get a reference to the remote platform proxy
	 * @return A PlatformProxy reference
	 */
	inline PlatformProxy const & GetRemotePlatformProxy() {
		return *rpp;
	}
#endif


private:

	/**
	 * @brief True if remote and local platform has been initialized
	 */
	bool platforms_initialized=false;

	/**
	 * @brief The logger used by the worker thread
	 */
	std::unique_ptr<bu::Logger> logger;

	/**
	 * @brief Pointer to local platform proxy
	 */
	std::unique_ptr<pp::LocalPlatformProxy> lpp;

#ifdef CONFIG_BBQUE_DIST_MODE
	/**
	 * @brief Pointer to remote platform proxy
	 */
	std::unique_ptr<pp::RemotePlatformProxy> rpp;
#endif // CONFIG_BBQUE_DIST_MODE
	/**
	 * @brief The set of flags related to pending platform events to handle
	 */
	std::bitset<PLATFORM_MANAGER_EV_COUNT> platformEvents;

	/**
	 * @brief The thread main code
	 */
	virtual void Task() override final;

	/**
	 * @brief The constructor. It is private due to singleton pattern
	 */
	PlatformManager();

	/**
	 * @brief Release all the platform proxy resources
	 */
	~PlatformManager();

	/**
	 * @brief Copy operator not allowed in singletons
	 */
	PlatformManager(PlatformManager const&) = delete;

	/**
	 * @brief Assigment-Copy operator not allowed in singletons
	 */
	void operator=(PlatformManager const&)  = delete;


	/**
	 * @brief The command handler callback function
	 */
	int CommandsCb(int argc, char *argv[]);

};

} // namespace bbque
#endif // PLATFORMMANAGER_H
