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

#include "bbque/pm/power_manager_gpu_arm_mali.h"

namespace bbque {

ARM_Mali_GPUPowerManager::ARM_Mali_GPUPowerManager() {


}

ARM_Mali_GPUPowerManager::~ARM_Mali_GPUPowerManager() {


}

/* Load and temperature */

PowerManager::PMResult
ARM_Mali_GPUPowerManager::GetLoad(
		br::ResourcePathPtr_t const & rp,
		uint32_t &perc) {


	return PMResult::OK;
}

PowerManager::PMResult
ARM_Mali_GPUPowerManager::GetTemperature(
		br::ResourcePathPtr_t const & rp,
		uint32_t &celsius) {


	return PMResult::OK;
}

/* Clock frequency */

PowerManager::PMResult
ARM_Mali_GPUPowerManager::GetClockFrequency(
		br::ResourcePathPtr_t const & rp,
		uint32_t &khz) {

	return PMResult::OK;
}


PowerManager::PMResult
ARM_Mali_GPUPowerManager::GetAvailableFrequencies(
		br::ResourcePathPtr_t const & rp,
		std::vector<unsigned long> & freqs) {

	return PMResult::OK;
}


PowerManager::PMResult
ARM_Mali_GPUPowerManager::SetClockFrequency(
		br::ResourcePathPtr_t const & rp,
		uint32_t khz) {

	return PMResult::OK;
}

/* Power consumption */

PowerManager::PMResult
ARM_Mali_GPUPowerManager::GetPowerUsage(
		br::ResourcePathPtr_t const & rp, uint32_t &mwatt) {

	return PMResult::OK;
}


/* Power states */

PowerManager::PMResult
ARM_Mali_GPUPowerManager::GetPowerState(
		br::ResourcePathPtr_t const & rp,
		uint32_t & state) {

	return PMResult::OK;
}

PowerManager::PMResult
ARM_Mali_GPUPowerManager::SetPowerState(
		br::ResourcePathPtr_t const & rp,
		uint32_t state) {

	return PMResult::OK;
}

} // namespace bbque

