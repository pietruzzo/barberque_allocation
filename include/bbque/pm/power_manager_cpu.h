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

#include "bbque/pm/power_manager.h"

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

};

}

#endif // BBQUE_POWER_MANAGER_CPU_H_
