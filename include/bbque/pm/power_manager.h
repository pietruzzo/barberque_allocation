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

#include <array>
#include <cstdint>
#include <vector>

#include "bbque/command_manager.h"
#include "bbque/pm/model_manager.h"
#include "bbque/utils/logging/logger.h"

namespace bu = bbque::utils;
namespace bw = bbque::pm;

namespace bbque {

namespace res {
	class ResourcePath;
	typedef std::shared_ptr<ResourcePath> ResourcePathPtr_t;
}
namespace br = bbque::res;


class PowerManager: public CommandHandler {

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
		ERR_RSRC_INVALID_PATH,
		/** Sensors error. e.g. cannot read a sensor*/
		ERR_SENSORS_ERROR
	};

	/**
	 * @enum Information classes that the module provides
	 */
	enum class InfoType: uint8_t {
		LOAD        = 0,
		TEMPERATURE = 1,
		FREQUENCY   = 2,
		POWER       = 3,
		CURRENT     = 4,
		VOLTAGE     = 5,
		PERF_STATE  = 6,
		POWER_STATE = 7,
		COUNT       = 8
	};

	/**
	 * @brief Data structure to store the amount of samples specified for the
	 * computation of mean value of the required (enabled) information
	 */
	typedef std::array<uint, int(InfoType::COUNT)> SamplesArray_t;
	/**
	 * @brief Keep track of the integer indices associated to the information
	 * types provided
	 */
	static std::array<InfoType, int(InfoType::COUNT)>   InfoTypeIndex;
	static std::array<const char *, int(InfoType::COUNT)> InfoTypeStr;

	enum class FanSpeedType {
		PERCENT = 0,
		RPM
	};

	static PowerManager & GetInstance();

	virtual ~PowerManager();

	/** Runtime activity load */

	virtual PMResult GetLoad(
		br::ResourcePathPtr_t const & rp, uint32_t &perc);


	/** Temperature */

	virtual PMResult GetTemperature(
		br::ResourcePathPtr_t const & rp, uint32_t &celsius);


	/** Clock frequency */

	virtual PMResult GetClockFrequency(
		br::ResourcePathPtr_t const & rp, uint32_t &khz);

	virtual PMResult GetClockFrequencyInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step);

	virtual PMResult GetAvailableFrequencies(
		br::ResourcePathPtr_t const & rp,
		std::vector<uint32_t> & freqs);

	virtual PMResult SetClockFrequency(
		br::ResourcePathPtr_t const & rp, uint32_t khz);

	virtual PMResult SetClockFrequency(
		br::ResourcePathPtr_t const & rp,
		uint32_t khz_min,
		uint32_t khz_max);


	virtual std::vector<std::string> const & GetAvailableFrequencyGovernors(
			br::ResourcePathPtr_t const & rp);

	virtual PMResult GetClockFrequencyGovernor(
			br::ResourcePathPtr_t const & rp,
			std::string & governor);

	virtual PMResult SetClockFrequencyGovernor(
			br::ResourcePathPtr_t const & rp,
			std::string const & governor);


	/** Voltage information */

	virtual PMResult GetVoltage(
		br::ResourcePathPtr_t const & rp, uint32_t &mvolt);

	virtual PMResult GetVoltageInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &mvolt_min,
		uint32_t &mvolt_max,
		uint32_t &mvolt_step);


	/**  Fan speed information */

	virtual PMResult GetFanSpeed(
		br::ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value);

	virtual PMResult GetFanSpeedInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step);

	virtual PMResult SetFanSpeed(
		br::ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value);

	virtual PMResult ResetFanSpeed(br::ResourcePathPtr_t const & rp);


	/** Power consumption information */

	virtual PMResult GetPowerUsage(
		br::ResourcePathPtr_t const & rp, uint32_t &mwatt);

	virtual PMResult GetPowerInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &mwatt_min,
		uint32_t &mwatt_max) ;


	/** Performance/power states */

	virtual PMResult GetPowerState(br::ResourcePathPtr_t const & rp, uint32_t & state);

	virtual PMResult GetPowerStatesInfo(
		br::ResourcePathPtr_t const & rp, uint32_t & min, uint32_t & max, int & step);

	virtual PMResult SetPowerState(br::ResourcePathPtr_t const & rp, uint32_t state);


	virtual PMResult GetPerformanceState(
		br::ResourcePathPtr_t const & rp, uint32_t &value);

	virtual PMResult GetPerformanceStatesCount(
		br::ResourcePathPtr_t const & rp, uint32_t &count);

	virtual PMResult SetPerformanceState(
		br::ResourcePathPtr_t const & rp, uint32_t value);

	int CommandsCb(int argc, char *argv[]);

protected:

	PowerManager();

	/**
	 * Command manager instance
	 */
	CommandManager * cm;

	/**
	 * P/T model manager
	 */
	bw::ModelManager * mm;

	/**
	 * @brief The logger used by the power manager.
	 */
	std::unique_ptr<bu::Logger> logger;

	/*
	 * @brief A GPU specific PM provider
	 *
	 * NOTE: this implementation supports a single GPU vendor, which must be
	 * defined at configuration time.
	 */
	std::unique_ptr<PowerManager> gpu;


	/*
	 * @brief A CPU specific PM provider
	 *
	 */
	std::unique_ptr<PowerManager> cpu;


	/**
	 * @brief Available CPU frequency governors
	 */
	std::vector<std::string> cpufreq_governors;

	/*
	 * @brief Command handler for setting a device fan speed
	 *
	 * @param rp The resource path referencing the device (e.g. sys0.gpu0)
	 * @param rp The percentage speed value to set
	 *
	 * @return 0 for success, a positive number otherwise
	 */
	int FanSpeedSetHandler(
		br::ResourcePathPtr_t const & rp, uint8_t speed_perc);
};

}

#endif // BBQUE_POWER_MANAGER_H_

