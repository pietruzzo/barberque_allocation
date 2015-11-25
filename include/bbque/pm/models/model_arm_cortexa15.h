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

#ifndef BBQUE_MODEL_ARM_CORTEXA15_H_
#define BBQUE_MODEL_ARM_CORTEXA15_H_

#include <string>

#include "bbque/pm/models/model.h"

#define BBQUE_MODEL_ARM_CORTEXA15_ID   "ARM Cortex A15"
#define BBQUE_MODEL_ARM_CORTEXA15_TPD  7200

namespace bbque  { namespace pm {

/**
 * @class ARM_CortexA15_Model
 *
 * @brief ARM Cortex A15 power-thermal model
 */
class ARM_CortexA15_Model: public Model {

public:

	/**
	 * @brief Constructor
	 * @param id Identifier string
	 * @param tpd Thermal-Power Design value in milliwatts
	 */
	ARM_CortexA15_Model(
			std::string id = BBQUE_MODEL_ARM_CORTEXA15_ID,
			uint32_t tpd   = BBQUE_MODEL_ARM_CORTEXA15_TPD):
		Model(id, tpd) {
	};

	/**
	 * @brief Destructor
	 */
	virtual ~ARM_CortexA15_Model() {};


	/*** Member functions to override ***/

	uint32_t GetPowerFromTemperature(
			uint32_t temp_mc,
			std::string const & freq_governor
				= BBQUE_PM_DEFAULT_CPUFREQ_GOVERNOR);

	uint32_t GetPowerFromSystemBudget(
			uint32_t power_mw,
			std::string const & freq_governor
				= BBQUE_PM_DEFAULT_CPUFREQ_GOVERNOR);


	uint32_t GetTemperatureFromPower(
			uint32_t power_mw,
			std::string const & freq_governor
				= BBQUE_PM_DEFAULT_CPUFREQ_GOVERNOR);

	float GetResourcePercentageFromPower(
			uint32_t power_mw,
			std::string const & freq_governor
				= BBQUE_PM_DEFAULT_CPUFREQ_GOVERNOR);


	uint32_t GetResourceFromPower(
			uint32_t power_mw,
			uint32_t total_amount,
			std::string const & freq_governor
				= BBQUE_PM_DEFAULT_CPUFREQ_GOVERNOR);


};

} // namespace pm

} // namespace bbque

#endif // BBQUE_MODEL_ARM_CORTEXA15_H_
