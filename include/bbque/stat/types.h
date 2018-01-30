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

/* Status filter description */
enum status_filter_t {
	FILTER_RESOURCE = 1,    // 00000001
	FILTER_APPLICATION = 2, // 00000010
	FILTER_SCHEDULE = 4     // 00000100
};

/* Status event description */
enum status_event_t {
	EVT_SCHEDULING = 1,     // 00000001
	EVT_APPLICATION = 2,    // 00000010
	EVT_RESOURCE = 4        // 00000100
};

struct task_t { // 13 Byte
	/* Serialization stuff */
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version){
		ar & id;
		ar & perf;
		ar & mapping;
	}
	/* Struct fields */
	uint32_t id;
	uint8_t perf; // In percentage
	res_bitset_t mapping;
};

struct app_status_t { // 32 Byte min
	/* Serialization stuff */
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version){
		ar & id;
		ar & name;
		ar & n_task;
		ar & tasks;
		ar & mapping;
	}
	/* Struct fields */
	uint32_t id;
	std::string name;
	uint32_t n_task;
	std::list<task_t> tasks; // List of tasks
	res_bitset_t mapping;
};

struct resource_status_t { // 15 Byte
	/* Serialization stuff */
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version){
		ar & id;
		ar & occupancy;
		ar & load;
		ar & power;
		ar & temp;
		ar & fans;
	}
	/* Struct fields */
	res_bitset_t id;
	uint8_t occupancy;
	uint8_t load;
	uint32_t power;
	uint32_t temp;
	uint32_t fans;
};

struct __attribute__((packed)) subscription_message_t { // 9 Byte
	uint32_t port_num;
	sub_bitset_t filter; // S|A|R|Reserved
	sub_bitset_t event; // S|A|R|Reserved
	uint16_t rate_ms;
	uint8_t mode; // 0 is subscribe; !0 is unsubscribe
};

struct status_message_t { // 59 Byte min
	/* Serialization stuff */
	template<class Archive>
	void serialize(Archive &ar, const unsigned int version){
		ar & n_app_status_msgs;
		ar & app_status_msgs;
		ar & n_res_status_msgs;
		ar & res_status_msgs;
	}
	/* Struct fields */
	uint32_t ts;
	uint32_t n_app_status_msgs;
	std::list<app_status_t> app_status_msgs;
	uint32_t n_res_status_msgs;
	std::list<resource_status_t> res_status_msgs;
};


} // namespace stat

} // namespace bbque

#endif // BBQUE_STAT_TYPES_H_