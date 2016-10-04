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

#ifndef BBQUE_BATTERY_H_
#define BBQUE_BATTERY_H_

#include <cstdint>
#include <fstream>
#include <string>

#include "bbque/pm/battery_config.h"

#include "bbque/utils/logging/logger.h"


namespace bu = bbque::utils;

namespace bbque {

/**
 * @class Battery
 * @brief The class is an interface towards a battery installed in the system
 */
class Battery  {


public:

	/**
	 * @brief The constructor
	 *
	 * @param name The name used in the directory under sysfs and procfs
	 * @param i_dir [optional] The sysfs path
	 * @param s_dir [optional] The procfs path
	 */
	Battery(
			std::string const & name,
			const char * i_dir = BBQUE_BATTERY_SYS_ROOT,
			const char * s_dir = BBQUE_BATTERY_PROC_ROOT);

	/**
	 * @brief The string identifier
	 *
	 * @return The sysfs directory name
	 */
	std::string const & StrId() const;

	/**
	 * @brief The battery technology
	 *
	 * @return A string describing the technology
	 */
	std::string const & GetTechnology() const;

	/**
	 * @brief Check if the battery object has been successfully initialized
	 *
	 * @return true if the battery object is ready, false otherwise
	 */
	bool IsReady() const;

	/**
	 * @brief Check the battery status
	 *
	 * @return true if the battery is discharging
	 */
	bool IsDischarging();

	/**
	 * @brief The voltage provided
	 *
	 * @return The voltage value in millivolts
	 */
	uint32_t GetVoltage();

	/**
	 * @brief The absorbed power consumption
	 *
	 * @return A power measure in milliwatts
	 */
	uint32_t GetPower();

	/**
	 * @brief The full capacity of the battery
	 *
	 * @return The capacity value in mAh
	 */
	unsigned long GetChargeFull() const;

	/**
	 * @brief The current charge of the battery in mAh
	 *
	 * @return The energy value in mAh
	 */
	unsigned long GetChargeMAh();

	/**
	 * @brief The current charge of the battery in percentage
	 *
	 * @return The percentage of charge
	 */
	uint8_t GetChargePerc();

	/**
	 * @brief The discharging rate of the battery
	 *
	 * @return The current value in mA drained from the battery
	 */
	uint32_t GetDischargingRate();

	uint32_t GetDischargingRateMax();

	uint32_t GetDischargingRateMin();

	/**
	 * @brief An estimation of the current remaining time
	 *
	 * @return The remaining time in seconds
	 */
	unsigned long GetEstimatedLifetime();

	/**
	 * @brief Log a status table
	 */
	void LogReportStatus();

	/**
	 * @brief Get a string showing the status bar
	 */
	std::string PrintChargeBar();

private:

	/*** The logger */
	std::unique_ptr<bu::Logger> logger;

	/*** The identifier name */
	std::string str_id;

	/*** sysfs path */
	std::string info_dir;

	/*** proc path */
	std::string status_dir;

	/*** File descriptor used for accessing interface files */
	std::ifstream fd;

	bool ready = false;


	/*** The battery technology */
	std::string technology;

	/*** The identifier name */
	unsigned long charge_full;

	/*** The supplied voltage */
	uint32_t voltage;


	uint32_t GetMilliUInt32From(std::string const & path);


#ifndef CONFIG_BBQUE_PM_NOACPI

	uint32_t ACPI_GetVoltage();

	uint32_t ACPI_GetDischargingRate();

#endif // CONFIG_BBQUE_PM_NOACPI

};

} // namespace bbque

#endif


