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


#ifndef BBQUE_POWER_MONITOR_H_
#define BBQUE_POWER_MONITOR_H_

#include <cstdint>
#include <map>

#include "bbque/command_manager.h"
#include "bbque/config.h"
#include "bbque/pm/power_manager.h"
#include "bbque/res/resources.h"
#include "bbque/utils/worker.h"
#include "bbque/utils/logging/logger.h"

#define POWER_MONITOR_NAMESPACE "bq.wm"

#define WM_EVENT_UPDATE      0
#define WM_EVENT_COUNT       1

#define WM_STRW  { 4,4,10,5,6,6,3,3}
#define WM_STRP  { 1,1, 0,1,1,1,1,1}

namespace bu = bbque::utils;
namespace br = bbque::res;

namespace bbque {


class PowerMonitor: public CommandHandler, bu::Worker {

public:

	/**
	 * @enum ExitCode_t
	 *
	 * Class specific return codes
	 */
	enum class ExitCode_t {
		/** Successful call */
		OK = 0,
		/** Not valid resource specified */
		ERR_RSRC_MISSING,
		/** A not specified error code */
		ERR_UNKNOWN
	};

	/** Power Monitor instance */
	static PowerMonitor & GetInstance();

	/**
	 * @brief Destructor
	 */
	virtual ~PowerMonitor();

	/**
	 * @brief Command handlers dispatcher
	 */
	int CommandsCb(int argc, char *argv[]);

	/**
	 * @brief Register the resources to monitor for collecting run-time
	 * power-thermal information
	 *
	 * @param rp Resource path of the resource(s)
	 * @param info_mask A bitset with the flags of the information to sample
	 *
	 * @return ERR_RSRC_MISSING if the resource path does not
	 * reference any resource, OK otherwise
	 */
	ExitCode_t Register(
			br::ResourcePathPtr_t rp,
			PowerManager::SamplesArray_t const & samples_window =
				{BBQUE_PM_DEFAULT_SAMPLES_WINSIZE}
	);

	/**
	 * @brief Start the monitoring of the power-thermal status
	 *
	 * @param period_ms Period of information sampling in milliseconds
	 */
	void Start(uint32_t period_ms = 0);

	/**
	 * @brief Stop the monitoring of the power-thermal status
	 */
	void Stop();

private:

	/*
	 * @brief Power manager instance
	 */
	PowerManager & pm;

	/**
	 * @brief Command manager instance
	 */
	CommandManager & cm;

	/**
	 * @brief The logger used by the power manager.
	 */
	std::unique_ptr<bu::Logger> logger;

	/**
	 * @brief The set of flags related to pending monitoring events to handle
	 */
	std::bitset<WM_EVENT_COUNT> events;


	/**
	 * @brief Structure to collect support information for the power
	 * monitoring activity
	 */
	struct PowerMonitorInfo_t {
		/** Resources to monitor */
		std::map<br::ResourcePathPtr_t, br::ResourcePtr_t> resources;

		bool started = false;
		/** Monitoring period (milliseconds) */
		uint32_t period_ms = 1000;
	} wm_info;

	/** Function pointer to PowerManager member functions */
	typedef PowerManager::PMResult (PowerManager::*PMfunc)
		(br::ResourcePathPtr_t const &, uint32_t &);

	/**
	 * @brief Array of Power Manager member functions
	 */
	std::array<PMfunc, size_t(PowerManager::InfoType::COUNT) > PowerMonitorGet;

	/*** Log messages format settings */
	std::array<int, size_t(PowerManager::InfoType::COUNT) > str_w = { WM_STRW };
	std::array<int, size_t(PowerManager::InfoType::COUNT) > str_p = { WM_STRP };

	/**
	 * @brief Constructor
	 */
	PowerMonitor();

	/**
	 * @brief Power Manager member functions initialization
	 */
	void Init();

	/**
	 * @brief Sample the power-thermal status information
	 */
	ExitCode_t Sample();

	/**
	 * @brief Periodic task
	 */
	virtual void Task();

};

} // namespace bbque

#endif // BBQUE_POWER_MONITOR_H_

