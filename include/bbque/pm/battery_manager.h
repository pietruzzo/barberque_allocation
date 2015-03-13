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

#ifndef BBQUE_BATTERY_MANAGER_H_
#define BBQUE_BATTERY_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "bbque/command_manager.h"
#include "bbque/pm/battery.h"
#include "bbque/utils/logging/logger.h"

namespace bbque {

typedef std::shared_ptr<Battery> BatteryPtr_t;


class BatteryManager: public CommandHandler {

public:

	/**
	 * @brief Batterymanager instance
	 */
	static BatteryManager & GetInstance();

	virtual ~BatteryManager();

	/**
	 * @brief Get a battery descriptor
	 *
	 * @param id The identifier number of the battery. Default value is 0,
	 * meaning that the first found battery is returned.
	 *
	 * @return A (shared) pointer to a Battery descriptor
	 */
	BatteryPtr_t GetBattery(uint8_t id = 0);

	/**
	 * @brief The generic command handler
	 */
	int CommandsCb(int argc, char *argv[]);

private:

	BatteryManager();

	/**
	 * @brief The logger
	 */
	std::unique_ptr<bu::Logger> logger;

	/**
	 * @brief The set of batteries avalable in the system
	 */
	std::vector<BatteryPtr_t> batteries;
};

} // namespace bbque

#endif
