#ifndef BBQUE_AGENT_PROXY_IF_H
#define BBQUE_AGENT_PROXY_IF_H

#define AGENT_PROXY_NAMESPACE "bq.gx"
#define AGENT_PROXY_CONFIG    "AgentProxy"

#include "bbque/pp/platform_description.h"
#include "bbque/plugins/agent_proxy_types.h"

namespace bbque
{
namespace plugins
{

using bbque::agent::ExitCode_t;

/**
 * @class AgentProxyIF
 *
 * @brief Interface for enabling the multi-agent configuration of
 * the BarbequeRTRM
 */
class AgentProxyIF {

public:

	AgentProxyIF() {}

	virtual ~AgentProxyIF() {}


	virtual void StartServer() = 0;

	virtual void StopServer() = 0;

	virtual void WaitForServerToStop() = 0;


	virtual void SetPlatformDescription(bbque::pp::PlatformDescription const * platform) {
		(void) platform;
	}

	// ----------------- Query status functions --------------------

	/**
	 * @brief GetResourceStatus
	 * @param path
	 * @param status
	 * @return
	 */
	virtual ExitCode_t GetResourceStatus(
		std::string const & resource_path,
		agent::ResourceStatus & status) = 0;

	/**
	 * @brief GetWorkloadStatus
	 * @param path
	 * @param status
	 * @return
	 */
	virtual ExitCode_t GetWorkloadStatus(
		std::string const & path, agent::WorkloadStatus & status) = 0;
	/**
	 * @param system_id
	 */
	virtual ExitCode_t GetWorkloadStatus(
		int system_id, agent::WorkloadStatus & status) = 0;

	/**
	 * @brief GetChannelStatus
	 * @param path
	 * @param status
	 * @return
	 */
	virtual ExitCode_t GetChannelStatus(
		std::string const & path, agent::ChannelStatus & status) = 0;
	/**
	 * @param system_id
	 */
	virtual ExitCode_t GetChannelStatus(
		int system_id, agent::ChannelStatus & status) = 0;


	// ------------- Multi-remote management functions ------------------

	/**
	 * @brief SendJoinRequest
	 * @param path
	 * @return
	 */
	virtual ExitCode_t SendJoinRequest(std::string const & path) = 0;
	/**
	 * @param system_id
	 */
	virtual ExitCode_t SendJoinRequest(int system_id) = 0;

	/**
	 * @brief SendDisjoinRequest
	 * @param path
	 * @return
	 */
	virtual ExitCode_t SendDisjoinRequest(std::string const & path) = 0;
	/**
	 * @param system_id
	 */
	virtual ExitCode_t SendDisjoinRequest(int system_id) = 0;


	// ----------- Scheduling / Resource allocation functions ----------

	/**
	* @brief SendScheduleRequest
	* @param path
	* @param request
	* @return
	*/
	virtual ExitCode_t SendScheduleRequest(
		std::string const & path,
		agent::ApplicationScheduleRequest const & request) = 0;

};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_REMOTE_PROXY_IF_H
