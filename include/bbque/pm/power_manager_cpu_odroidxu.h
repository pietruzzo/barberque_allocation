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
#ifndef BBQUE_POWER_MANAGER_CPU_ODROIDXU_H_
#define BBQUE_POWER_MANAGER_CPU_ODROIDXU_H_

#include "bbque/pm/power_manager_cpu.h"
#include "bbque/pm/power_manager_odroidxu.h"
#include "bbque/res/resource_path.h"

namespace bbque {

class ODROID_XU_CPUPowerManager: public CPUPowerManager {

public:

	ODROID_XU_CPUPowerManager();

	virtual ~ODROID_XU_CPUPowerManager();


	/** Power consumption  */

	PMResult GetPowerUsage(br::ResourcePathPtr_t const & rp, uint32_t & mwatt);

	/** Temperature  */

	PMResult GetTemperature(br::ResourcePathPtr_t const & rp, uint32_t & celsius);

private:

	/**
	 * Prefix path for the sensors of the clusters. The index of the array
	 * is supposed to be the CPU core id. e.g., cluster_prefix[5] = 'A15 path..'
	 */
	std::string GetSensorsPrefixPath(br::ResourcePathPtr_t const & rp);
};

} // namespace bbque

#endif // BBQUE_POWER_MANAGER_CPU_ODROIDXU_H_
