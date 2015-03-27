/*
 * Copyright (C) 2015  Politecnico di Milano
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
#ifndef BBQUE_POWER_MANAGER_GPU_ARM_MALI_H_
#define BBQUE_POWER_MANAGER_GPU_ARM_MALI_H_

#include "bbque/pm/power_manager.h"

#define BBQUE_ARM_MALI_SYS_DIR     "/sys/bus/platform/drivers/mali/"
#define BBQUE_ARM_MALI_SYS_DEVDIR  "11800000.mali"
#define BBQUE_ARM_MALI_SYS_PATH \
	BBQUE_ARM_MALI_SYS_DIR##BBQUE_ARM_MALI_SYS_DEVDIR

namespace bu = bbque::utils;

namespace bbque {

/**
 * @class ARM_Mali_GPUPowerManager
 *
 * This class provides the abstraction towards the management of the ARM Mali
 * GPUs. The current version support platform including a single GPU device.
 */
class ARM_Mali_GPUPowerManager: public PowerManager {

public:

	ARM_Mali_GPUPowerManager();

	virtual ~ARM_Mali_GPUPowerManager();

	/** Runtime activity load */

	PMResult GetLoad(
		br::ResourcePathPtr_t const & rp, uint32_t &perc);


	/** Temperature */

	PMResult GetTemperature(
		br::ResourcePathPtr_t const & rp, uint32_t &celsius);


	/** Clock frequency */

	PMResult GetClockFrequency(
		br::ResourcePathPtr_t const & rp, uint32_t &khz);

	PMResult GetAvailableFrequencies(
		br::ResourcePathPtr_t const & rp,
		std::vector<unsigned long> & freqs);

	PMResult SetClockFrequency(
		br::ResourcePathPtr_t const & rp, uint32_t khz);


	/** Power consumption information */

	PMResult GetPowerUsage(
		br::ResourcePathPtr_t const & rp, uint32_t &mwatt);


	/** Performance/power states */

	PMResult GetPowerState(br::ResourcePathPtr_t const & rp, uint32_t & state);

	PMResult SetPowerState(br::ResourcePathPtr_t const & rp, uint32_t state);

protected:

	/**
	 * @brief The vector including all the supported clock frequency values
	 */
	std::vector<unsigned long> freqs;
};

}

#endif // BBQUE_POWER_MANAGER_GPU_ARM_MALI_H_

