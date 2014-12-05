/*
 * Copyright (C) 2014  Politecnico di Milano
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

#ifndef BBQUE_POWER_MANAGER_CPU_H_
#define BBQUE_POWER_MANAGER_CPU_H_

#include <map>
#include <vector>

#include "bbque/pm/power_manager.h"
#include <cpufreq.h>

using namespace bbque::res;

namespace bbque {

/**
 * @class CPUPowerManager
 *
 * Provide power management related API for AMD GPU devices, by extending @ref
 * PowerManager class.
 */
class CPUPowerManager: public PowerManager {

public:

	enum class ExitStatus {
		/** Successful call */
		OK = 0,
		/** A not specified error code */
		ERR_GENERIC
	};

	/**
	 * @see class PowerManager
	 */
	CPUPowerManager();

	~CPUPowerManager();

	/**
	 * @see class PowerManager
	 */
	PMResult GetLoad(ResourcePathPtr_t const & rp, uint32_t & perc);

	/**
	 * @see class PowerManager
	 */
	PMResult GetClockFrequency(ResourcePathPtr_t const & rp, uint32_t &khz);

	/**
	 * @see class PowerManager
	 */
	PMResult GetTemperature(ResourcePathPtr_t const & rp, uint32_t &celsius);

	/**
	 * @see class PowerManager
	 */
	PMResult GetAvailableFrequencies(
			ResourcePathPtr_t const & rp, std::vector<unsigned long> &freqs);

private:

	/*** Mapping processing elements / CPU cores */
	std::map<int,int> core_ids;

	/**
	 * Save the information of a single /proc/stat sampling
	 */
	struct LoadInfo {
		/// Total activity time (jitters)
		int32_t total = 0;
		/// Idle time (jitters)
		int32_t idle = 0;
	};

	/**
	 *  Sample CPU activity samples from /proc/stat
	 */
	PMResult GetLoadCPU(br::ResID_t cpu_core_id, uint32_t & load);

	/**
	 *  Sample CPU activity samples from /proc/stat
	 */
	ExitStatus GetLoadInfo(LoadInfo *info, br::ResID_t cpu_core_id);

	/**
	 * @brief Extract the CPU core id
	 *
	 * @param rp Constant reference to ResourcePath pointer
	 *
	 * @return the ID number
	 */
	int GetCPU(ResourcePathPtr_t const & rp);

};

}

#endif // BBQUE_POWER_MANAGER_CPU_H_
