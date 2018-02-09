/*
 * Copyright (C) 2017  Politecnico di Milano
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

#ifndef BBQUE_STAT_TYPES_H_
#define BBQUE_STAT_TYPES_H_

#define MAX_APP_NAME_LEN 16

#include <cstdint>
#include <string>
#include <list>
#include <bitset>

namespace bbque { namespace stat {

using res_bitset_t = uint64_t;
using sub_bitset_t = uint8_t;

#define BITSET_OFFSET_SYS       52
#define BITSET_OFFSET_UNIT_TYPE 48
#define BITSET_OFFSET_UNIT_ID   32
#define BITSET_OFFSET_PE_TYPE   31
#define BITSET_OFFSET_PE_ID     0

#define BITSET_LEN_SYS       12
#define BITSET_LEN_UNIT_TYPE 4
#define BITSET_LEN_UNIT_ID   16
#define BITSET_LEN_PE_TYPE   1
#define BITSET_LEN_PE_ID     31

#define BITSET_LEN_RES       64
#define BITSET_LEN_SUB       8

/* Status filter description */
enum status_filter_t {
	FILTER_RESOURCE = 1,    // 00000001
	FILTER_APPLICATION = 2, // 00000010
	FILTER_SCHEDULE = 4     // 00000100
};

/* Status event description */
enum status_event_t {
	NO_EVT = 0,             // 00000000
	EVT_SCHEDULING = 1,     // 00000001
	EVT_APPLICATION = 2,    // 00000010
	EVT_RESOURCE = 4        // 00000100
};

struct task_status_t { // 13 Byte
	/* Serialization stuff */
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version){
		(void) version;
		ar & id;
		ar & perf;
		ar & mapping;
	}
	/* Struct fields */
	uint32_t id;             /// Task identification number
	uint8_t perf;            /// Runtime performance (%) w.r.t. the goal
	res_bitset_t mapping;    /// Mapping to computing units
};

struct app_status_t { // 32 Byte min
	/* Serialization stuff */
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version){
		(void) version;
		ar & id;
		ar & name;
		ar & n_task;
		ar & tasks;
		ar & mapping;
	}
	/* Struct fields */
	uint64_t id;            /// Identification number
	std::string name;       /// Binary name
	uint32_t n_task;        /// Number of tasks included
	std::list<task_status_t> tasks; /// Per-task information
	res_bitset_t mapping;  /// Task mappings
};

struct resource_status_t { // 15 Byte
	/* Serialization stuff */
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version){
		(void) version;
		ar & id;
		ar & occupancy;
		ar & load;
		ar & power;
		ar & temp;
		ar & fans;
	}
	/* Struct fields */
	res_bitset_t id;
	uint8_t occupancy;   /// Amount assigned by the resource manager
	uint8_t load;        /// Utilization observerd at runtime
	uint32_t power;      /// Power consumption
	uint32_t temp;       /// Current temperature
	uint32_t fans;       /// Fan speed
};

struct __attribute__((packed)) subscription_message_t { // 9 Byte
	uint32_t port_num;    /// Client-side port
	sub_bitset_t filter;  /// S|A|R|Reserved
	sub_bitset_t event;   /// S|A|R|Reserved
	uint16_t rate_ms;     /// Periodic update requirement
	uint8_t mode;         /// 0=subscribe; !0=unsubscribe
};

struct status_message_t { // 59 Byte min
	/* Serialization stuff */
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version){
		(void) version;
		ar & ts;
		ar & n_app_status_msgs;
		ar & app_status_msgs;
		ar & n_res_status_msgs;
		ar & res_status_msgs;
	}
	/* Struct fields */
	uint32_t ts;                   /// Timestamp
	uint32_t n_app_status_msgs;    /// Number of (application) status messages
	std::list<app_status_t> app_status_msgs; /// Application status messages
	uint32_t n_res_status_msgs;    /// Number of (resource) status messages
	std::list<resource_status_t> res_status_msgs; /// Resource status messages
};


} // namespace stat

} // namespace bbque

#endif // BBQUE_STAT_TYPES_H_