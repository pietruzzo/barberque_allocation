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


#ifndef BBQUE_SYSTEM_MODEL_H_
#define BBQUE_SYSTEM_MODEL_H_

#include <cstdint>
#include <string>

#include "bbque/pm/models/model.h"


namespace bbque  { namespace pm {


/**
 * @class SystemModel
 *
 * @brief Base class of the power-thermal model of a system platform
 */
class SystemModel {

public:

	/**
	 * @brief Constructor
	 * @param id Identifier string
	 */
	SystemModel(std::string _id = "system"): id(_id) {}

	virtual ~SystemModel() {}

	/**
	 * @brief Model identifier
	 * @return Identifier string
	 */
	inline std::string const & GetID() const {
		return id;
	}

	/**
	 * @brief The amount of power consumption due to the computing resources
	 * activity, given the overall system power consumption
	 *
	 * @return A power consumption value in milli-watts
	 */
	virtual uint32_t GetResourcePowerFromSystem(
			uint32_t sys_power_budget_mw,
			std::string const & freq_governor
				= BBQUE_PM_DEFAULT_CPUFREQ_GOVERNOR) const;

protected:

	std::string id;

};

} // namespace pm

} // namespace bbque

#endif // BBQUE_SYSTEM_MODEL_H_


