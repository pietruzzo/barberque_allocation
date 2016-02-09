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


#ifndef BBQUE_OMPI_TYPES
#define BBQUE_OMPI_TYPES

#define BBQ_CMD_FINISHED       -2
#define BBQ_CMD_NONE           -1
#define BBQ_CMD_NODES_REQUEST   0
#define BBQ_CMD_NODES_REPLY     1
#define BBQ_CMD_TERMINATE       2

#include <cstdint>

/***********  Common   ************/
struct local_bbq_cmd_t {
	uint32_t jobid;			/* The jobid from ompi  */
	uint8_t cmd_type;		/* Command number: */
							/* Other things here? */
};
typedef struct local_bbq_cmd_t local_bbq_cmd_t;

/***********  OpenMPI -> BBQ   ************/
struct local_bbq_job_t {
	uint32_t jobid;
	uint32_t slots_requested;
};
typedef struct local_bbq_job_t local_bbq_job_t;


/***********  BBQ -> OpenMPI  ************/
struct local_bbq_res_item_t {
	uint32_t jobid;
	char hostname[256];					/* orte_node_t / name */
	int32_t slots_available;			/* orte_node_t / slots */
	uint8_t more_items;					/* boolean flag */
};
typedef struct local_bbq_res_item_t local_bbq_res_item_t;


#endif // BBQUE_OMPI_TYPES
