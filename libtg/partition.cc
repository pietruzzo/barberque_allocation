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

#include "bbque/pp/mango_platform_description.h"
#include "tg/partition.h"
#include "bbque/utils/utility.h"

namespace bbque {



	int Partition::GetMemoryBank(BufferPtr_t buff) const {
		return buffers_map.at(buff);	// It returns out_of_range exception if not present
	}


	int Partition::GetUnit(TaskPtr_t task) const {
		return tasks_map.at(task);	// It returns out_of_range exception if not present
	}

}
