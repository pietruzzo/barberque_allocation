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

#ifndef BBQUE_POWER_MANAGER_CPU_FREESCALE_IMX6_H_
#define BBQUE_POWER_MANAGER_CPU_FREESCALE_IMX6_H_

#include "bbque/pm/power_manager_cpu.h"
#include "bbque/utils/iofs.h"

#undef BBQUE_LINUX_SYS_CPU_THERMAL
#define BBQUE_LINUX_SYS_CPU_THERMAL  "/sys/class/thermal/thermal_zone0/temp"

namespace br = bbque::res;
namespace bu = bbque::utils;

namespace bbque {

/**
 * @class ARM_CortexA9_CPUPowerManager
 *
 * @brief Provide CPU power management API for Freescale iMX6 boards,
 * by extending @ref PowerManager class.
 */
class ARM_CortexA9_CPUPowerManager: public CPUPowerManager {

public:

	ARM_CortexA9_CPUPowerManager(){};

	~ARM_CortexA9_CPUPowerManager(){};

	/**
	 * @brief Get the CPU temperature
	 *
	 * Only one thermal sensor is available on the board
	 */
	inline PMResult GetTemperature(
			br::ResourcePathPtr_t const & rp, uint32_t &celsius) {
		(void) rp;
		bu::IoFs::ExitCode_t result;
		result = bu::IoFs::ReadIntValueFrom<uint32_t>(
				BBQUE_LINUX_SYS_CPU_THERMAL, celsius);
		if (result != bu::IoFs::OK) return PMResult::ERR_RSRC_INVALID_PATH;
		return PMResult::OK;
	}

};

} // namespace bbque

#endif // BBQUE_POWER_MANAGER_CPU_FREESCALE_IMX6_H_
