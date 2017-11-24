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

#ifndef BBQUE_STATS_TYPES_H_
#define BBQUE_STATS_TYPES_H_

#define MAX_APP_NAME_LEN 16

namespace bbque { namespace stats {

typedef uint64_t res_bitset_t;
typedef uint8_t sub_bitset_t;

typedef struct task { // 13 Byte
	uint32_t id;
	uint8_t perf; // In percentage
	res_bitset_t mapping;
}task_t;

struct  app_status{ // 32 Byte min
	uint32_t ts;
	uint32_t id;
	char name[MAX_APP_NAME_LEN];
	uint32_t n_task;
	task_t * tasks; // List of tasks
	res_bitset_t mapping;
};

struct resource_status{ //15 Byte
	uint32_t ts;
	res_bitset_t id;
	uint8_t occupancy;
	uint8_t load;
	uint32_t power;
	uint8_t temp;
};

struct subscription{
	res_bitset_t filter; // S|A|R|Reserved
	res_bitset_t event; // S|A|R|Reserved
	uint16_t rate_ms;
	uint8_t mode; // 0 is subscribe; !0 is unsubscribe
};

} // namespace stats

} // namespace bbque

#endif // BBQUE_STATS_TYPES_H_