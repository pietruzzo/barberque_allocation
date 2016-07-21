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

/***********************************************
 * Odroid XU-3 board
 ***********************************************/
#ifdef CONFIG_TARGET_ODROID_XU
#include "bbque/pm/power_manager_odroidxu.h"
// State attributes
#define BBQUE_ARM_MALI_SYS_FREQ     BBQUE_ODROID_SYS_DIR_GPU"/clock"
#define BBQUE_ARM_MALI_SYS_FREQS    BBQUE_ODROID_SYS_DIR_GPU"/dvfs_table"
#define BBQUE_ARM_MALI_SYS_DVFS     BBQUE_ODROID_SYS_DIR_GPU"/dvfs"
#define BBQUE_ARM_MALI_SYS_LOAD     BBQUE_ODROID_SYS_DIR_GPU"/utilization"
#define BBQUE_ARM_MALI_SYS_WSTATE   BBQUE_ODROID_SYS_DIR_GPU"/power_state"
// Sensors
#define BBQUE_ARM_MALI_SYS_VOLTAGE  BBQUE_ODROID_SENSORS_DIR_GPU"/sensor_V"
#define BBQUE_ARM_MALI_SYS_CURRENT  BBQUE_ODROID_SENSORS_DIR_GPU"/sensor_I"
#define BBQUE_ARM_MALI_SYS_POWER    BBQUE_ODROID_SENSORS_DIR_GPU"/sensor_W"
#endif // CONFIG_TARGET_ODROID_XU

namespace bu = bbque::utils;

namespace bbque {

/**
 * @class ARM_Mali_GPUPowerManager
 *
 * @brief This class provides the abstraction towards the management of
 * the ARM MaliGPUs. The current version support platform including a
 * single GPU device.
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

	PMResult GetVoltage(
		br::ResourcePathPtr_t const & rp, uint32_t &mvolt);

	/** Performance/power states */

	PMResult GetPowerState(br::ResourcePathPtr_t const & rp, uint32_t & state);

	PMResult SetPowerState(br::ResourcePathPtr_t const & rp, uint32_t state);

protected:

	/**
	 * @brief The vector including all the supported clock frequency values
	 */
	std::vector<unsigned long> freqs;

	void InitAvailableFrequencies();
};

}

#endif // BBQUE_POWER_MANAGER_GPU_ARM_MALI_H_

