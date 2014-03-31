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


#ifndef BBQUE_POWER_MANAGER_H_
#define BBQUE_POWER_MANAGER_H_

#include <cstdint>

#include "bbque/res/resource_path.h"
#include "bbque/utils/logging/logger.h"

#define POWER_MANAGER_NAMESPACE "bq.pm"

namespace br = bbque::res;
namespace bu = bbque::utils;

namespace bbque {


class PowerManager {

public:
	enum class PMResult {
		/** Successful call */
		OK = 0,
		/** A not specified error code */
		ERR_UNKNOWN,
		/** If something requires initialization and it is missing */
		ERR_NOT_INITIALIZED,
		/** Function not implemented in the current platform */
		ERR_API_NOT_SUPPORTED,
		/** Parameter with invalid value provided */
		ERR_API_INVALID_VALUE,
		/** Low level firmware version unsupported */
		ERR_API_VERSION,
		/** Information not provided for the resource specified */
		ERR_INFO_NOT_SUPPORTED,
		/** */
		ERR_RSRC_INVALID_PATH
	};

	enum class FanSpeedType {
		PERCENT = 0,
		RPM
	};

	static PowerManager & GetInstance(
		ResourcePathPtr_t rp, std::string const & vendor = "");


	virtual ~PowerManager();


	/** */
	std::string const & GetVendor() {
		return vendor;
	}


	/** Runtime activity load */

	virtual PMResult GetLoad(
		ResourcePathPtr_t const & rp, uint32_t &perc);


	/** Temperature */

	virtual PMResult GetTemperature(
		ResourcePathPtr_t const & rp, uint32_t &celsius);


	/** Clock frequency */

	virtual PMResult GetClockFrequency(
		ResourcePathPtr_t const & rp, uint32_t &khz);

	virtual PMResult GetClockFrequencyInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step);

	virtual PMResult SetClockFrequency(
		ResourcePathPtr_t const & rp, uint32_t khz);


	/** Voltage information */

	virtual PMResult GetVoltage(
		ResourcePathPtr_t const & rp, uint32_t &mvolt);

	virtual PMResult GetVoltageInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &mvolt_min,
		uint32_t &mvolt_max,
		uint32_t &mvolt_step);


	/**  Fan speed information */

	virtual PMResult GetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value);

	virtual PMResult GetFanSpeedInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step);

	virtual PMResult SetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value);

	virtual PMResult ResetFanSpeed(ResourcePathPtr_t const & rp);


	/** Power consumption information */

	virtual PMResult GetPowerUsage(
		ResourcePathPtr_t const & rp, uint32_t &mwatt);

	virtual PMResult GetPowerInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &mwatt_min,
		uint32_t &mwatt_max) ;


	/** Performance/power states */

	virtual PMResult GetPowerState(ResourcePathPtr_t const & rp, int & state);

	virtual PMResult GetPowerStatesInfo(
		ResourcePathPtr_t const & rp, int & min, int & max, int & step);

	virtual PMResult SetPowerState(ResourcePathPtr_t const & rp, int state);


	virtual PMResult GetPerformanceState(
		ResourcePathPtr_t const & rp, int &value);

	virtual PMResult GetPerformanceStatesCount(
		ResourcePathPtr_t const & rp, int &count);

	virtual PMResult SetPerformanceState(
		ResourcePathPtr_t const & rp, int value);

protected:

	PowerManager(ResourcePathPtr_t rp, std::string const & vendor);

	ResourcePathPtr_t rsrc_path_domain;

	std::string const & vendor;

	bool initialized = false;

	/**
	 * @brief The logger used by the power manager.
	 */
	std::unique_ptr<bu::Logger> logger;

};

}

#endif // BBQUE_POWER_MANAGER_H_

