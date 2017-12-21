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

namespace bbque { namespace stat {

#define STAT_BITSET_RESOURCE 1 // 00000001
#define STAT_BITSET_APPLICATION 2 // 00000010
#define STAT_BITSET_SCHEDULE  4 // 00000100

using res_bitset_t = uint64_t;
using sub_bitset_t = uint8_t;

struct task_t { // 13 Byte
	uint32_t id;
	uint8_t perf; // In percentage
	res_bitset_t mapping;
};

struct app_status_t { // 32 Byte min
	uint32_t id;
	char name[MAX_APP_NAME_LEN];
	uint32_t n_task;
	task_t * tasks; // List of tasks
	res_bitset_t mapping;
};

struct resource_status_t { // 15 Byte
	res_bitset_t id;
	uint8_t occupancy;
	uint8_t load;
	uint32_t power;
	uint8_t temp;
};

struct __attribute__((packed)) subscription_message_t { // 9 Byte
	uint32_t port_num;
	sub_bitset_t filter; // S|A|R|Reserved
	sub_bitset_t event; // S|A|R|Reserved
	uint16_t rate_ms;
	uint8_t mode; // 0 is subscribe; !0 is unsubscribe
};

struct status_message_t { // 59 Byte min
	uint32_t ts;
	uint32_t n_app_status_msgs;
	app_status_t* app_status_msgs;
	uint32_t n_res_status_msgs;
	resource_status_t* res_status_msgs;
};


} // namespace stat

} // namespace bbque

#endif // BBQUE_STAT_TYPES_H_