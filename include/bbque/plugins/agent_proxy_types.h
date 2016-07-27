
#ifndef BBQUE_AGENT_PROXY_TYPES_H_
#define BBQUE_AGENT_PROXY_TYPES_H_

#include <ctype.h>
#include <bitset>
#include <map>
#include <string>

#include "bbque/config.h"
#include "bbque/res/resource_type.h"

namespace bbque
{
namespace agent
{

/**
 * @enum ExitCode_t
 */
enum class ExitCode_t {
	OK,
	AGENT_UNREACHABLE,
	AGENT_DISCONNECTED,
	REQUEST_REJECTED,
	PROXY_NOT_READY
};

/**
 * @struct ResourceStatus
 */
struct ResourceStatus {
	uint64_t total;
	uint64_t used;
	int32_t power_mw;
	int32_t temperature;
	int16_t degradation;
};

/**
 * @struct WorkloadStatus
 */
struct WorkloadStatus {
	uint32_t nr_apps;
	uint32_t nr_running;
	uint32_t nr_ready;
};

/**
 * @struct ChannelStatus
 */
struct ChannelStatus {
	bool connected;
	int32_t latency_ms;
};

/**
 * @struct ApplicationScheduleRequest
 */
struct ApplicationScheduleRequest {
	int16_t app_id;
	std::string app_name;
	int16_t awm_id;
	struct {
		std::map<bbque::res::ResourceType, uint64_t> amount;
		std::map<bbque::res::ResourceType, std::bitset<BBQUE_MAX_R_ID_NUM+1>> binding_set;
	} resources;
};

} // namespace agent
} // namespace bbque

#endif // BBQUE_REMOTE_PROXY_TYPES_H_
