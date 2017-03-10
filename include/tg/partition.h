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


#ifndef BBQUE_TG_PARTITION_H
#define BBQUE_TG_PARTITION_H

#include <memory>
#include <map>
#include <cstdint>

#include "bbque/pp/platform_description.h"
#include "bbque/tg/task_graph.h"

namespace bbque {

class Partition {

public:

	Partition(uint32_t id) : partition_id(id) { }

	~Partition() {}


	inline uint32_t GetPartitionId() const noexcept {
		return this->partition_id;
	}

	inline uint_fast8_t GetMMScore() const noexcept {
		return this->mm_score;
	}

	inline void SetMMScore(uint_fast8_t score) noexcept {
		this->mm_score = score;
	}

	inline uint_fast8_t GetPMScore() const noexcept {
		return this->pm_score;
	}

	inline void SetPMScore(uint_fast8_t score) noexcept {
		this->pm_score = score;
	}

	inline void MapTask(TaskPtr_t task, int unit) noexcept {
		this->tasks_map.insert(std::make_pair(task, unit));
	}

	inline void MapBuffer(BufferPtr_t task, int unit) noexcept {
		this->buffers_map.insert(std::make_pair(task, unit));
	}

	/**
	 * @brief Get the id of the memory bank as an identifier number associated by the HN library
	 * @except std::out_of_range if the buffer is not present in the mapping
	 */
	int GetMemoryBank(BufferPtr_t buff) const;

	/**
	 * @brief Get the multicore processor assigned to the task
	 * @except std::out_of_range if the task is not present in the mapping
	 */
	int GetUnit(TaskPtr_t task) const;

	/**
	 * @brief Get the id of the unit assigned to the task.
	 * @note In the case of MangoTile processor it returns the tile number
	 * @except std::out_of_range if the task is not present in the mapping
	 */
	int GetUnitId(TaskPtr_t task) const;

private:
	const uint32_t partition_id;	/** The internal identifier returned by HN library */
	uint_fast8_t mm_score;	/** The score index [0;100] provided by the MemoryManager */
	uint_fast8_t pm_score;	/** The score index [0;100] provided by the PowerManager */

	std::shared_ptr<TaskGraph> tg;	/** The pointer to the associated TaskGraph */
	
	std::map<TaskPtr_t, int> tasks_map;
	std::map<BufferPtr_t, int> buffers_map;

};

} // namespace bbque

#endif // BBQUE_TG_PARTITION_H
