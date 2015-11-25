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

#ifndef BBQUE_SYSTEM_MODEL_ODROID_XU3_H_
#define BBQUE_SYSTEM_MODEL_ODROID_XU3_H_

#include <cmath>
#include <string>

#include "bbque/pm/models/system_model.h"

#define BBQUE_SYSMODEL_ODROID_XU3 	"ODROID XU3"

namespace bbque  { namespace pm {


/**
 * @class SystemModel
 *
 * @brief Base class of the power-thermal model of a system
 */
class ODROID_XU3_SystemModel: public SystemModel {

public:

	/**
	 * @brief Constructor
	 * @param id Identifier string
	 */
	ODROID_XU3_SystemModel(std::string _id = BBQUE_SYSMODEL_ODROID_XU3):
		SystemModel(_id) {};

	virtual ~ODROID_XU3_SystemModel() {};


	/*** Functions to override ***/

	uint32_t GetResourcePowerFromSystem(
			uint32_t sys_power_budget_mw,
			std::string const & freq_governor
				= BBQUE_PM_DEFAULT_CPUFREQ_GOVERNOR) const;

};

} // namespace pm

} // namespace bbque

#endif // BBQUE_SYSTEM_MODEL_ODROID_XU3_H_


