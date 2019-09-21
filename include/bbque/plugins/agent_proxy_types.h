/*
 * Copyright (C) 2016  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BBQUE_AGENT_PROXY_TYPES_H_
#define BBQUE_AGENT_PROXY_TYPES_H_

#include <ctype.h>
#include <bitset>
#include <map>
#include <string>
#include <list>

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
	int32_t load;
};

/**
 * @struct WorkloadStatus
 */
struct WorkloadStatus {
	uint32_t nr_ready;
	uint32_t nr_running;
};

/**
 * @struct ChannelStatus
 */
struct ChannelStatus {
	bool connected;
	double latency_ms;
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

/**
 * @struct ResourceAllocation
 * trans_id: identifier of the transaction
 * memory: amount of memory requested
 * proc: list of required resources
 */

struct ResourceAllocation {
	int32_t trans_id;
	int32_t memory;
	typedef struct ProcessingUnitsRequest {
		std::string arch;
		int32_t num;
	} ProcessingUnitsRequest;
	std::list<ProcessingUnitsRequest> proc;
};

/**
 * @struct AgreementReply
 * proc_paths: list of paths of required resources
 */

struct AgreementReply {
	typedef struct Paths {
		std::string arch;
		std::string path;
	} Paths;
	std::list<Paths> proc_paths;
};

} // namespace agent
} // namespace bbque

#endif // BBQUE_REMOTE_PROXY_TYPES_H_
