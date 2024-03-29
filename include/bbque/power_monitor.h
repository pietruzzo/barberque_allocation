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

#ifndef BBQUE_POWER_MONITOR_H_
#define BBQUE_POWER_MONITOR_H_

#include <cstdint>
#include <fstream>
#include <map>

#include "bbque/command_manager.h"
#include "bbque/config.h"
#include "bbque/configuration_manager.h"
#include "bbque/resource_manager.h"
#include "bbque/pm/battery_manager.h"
#include "bbque/pm/power_manager.h"
#include "bbque/res/resources.h"
#include "bbque/utils/deferrable.h"
#include "bbque/utils/worker.h"
#include "bbque/utils/logging/logger.h"
#include "bbque/trig/trigger.h"
#include "bbque/data_manager.h"

#define POWER_MONITOR_NAMESPACE "bq.wm"

#define WM_DEFAULT_PERIOD_MS    1000
#define WM_OPT_REQ_TIME_FACTOR     4

#define WM_EVENT_UPDATE      0
#define WM_EVENT_COUNT       1

#define WM_STRW  { 5,10,10,9,6,3,3,3}
#define WM_STRP  { 1, 1, 0,1,1,1,1,1}

namespace bu = bbque::utils;
namespace br = bbque::res;

namespace bbque {


class PowerMonitor: public CommandHandler, bu::Worker {

public:

	/**
	 * @enum ExitCode_t
	 * @brief Class specific return codes
	 */
	enum class ExitCode_t {
		OK = 0,           /** Successful call */
		ERR_RSRC_MISSING, /** Not valid resource specified */
		ERR_UNKNOWN       /** A not specified error code   */
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
	 * @return ERR_RSRC_MISSING if the resource path does not
	 * reference any resource, OK otherwise
	 */
	ExitCode_t Register(
			br::ResourcePathPtr_t rp,
			PowerManager::SamplesArray_t const & samples_window =
				{BBQUE_PM_DEFAULT_SAMPLES_WINSIZE}
	);

	ExitCode_t Register(
			const std::string & rp_str,
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

	/**
	 * @brief Thermal theshold
	 *
	 * @return The temperature in Celsius degree
	 */
	inline uint32_t GetThermalThreshold() const {
		return GetThreshold(PowerManager::InfoType::TEMPERATURE);
	}

	/**
	 * @brief Get the current threshold
	 *
	 * @return For TEMP the temperature in Celsius degree.
	 * For POWER the power consumption upper bound.
	 * For BATTERY_LEVEL the charge level under which a policy execution could be triggered.
	 * For BATTERY_RATE the maximum discharging rate tolerated.
	 */
	inline uint32_t GetThreshold(PowerManager::InfoType t) const {
		auto v = triggers.find(t);
		if (unlikely(v == triggers.end())) return 0;
		return v->second->threshold_high;
	}

#ifdef CONFIG_BBQUE_PM_BATTERY
	/**
	 * @brief System lifetime left (in seconds)
	 *
	 * @return Chrono duration object (seconds) with the count of the
	 * remaining seconds
	 */
	inline std::chrono::seconds GetSysLifetimeLeft() const {
		std::chrono::system_clock::time_point now =
			std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::seconds>(
				sys_lifetime.target_time - now);
	}
#endif

	/**
	 * @brief System power budget, given the target lifetime set
	 *
	 * @return The power value in milliwatts; 0: No target set; -1: Always on
	 * mode required
	 */
	int32_t GetSysPowerBudget();

	/**
	 * @brief Return the length of the sampling period (in milliseconds)
	 */
	uint32_t GetPeriodLengthMs() const {
		return wm_info.period_ms;
	}

private:

	/**
	 * @brief Power manager instance
	 */
	PowerManager & pm;

#ifdef CONFIG_BBQUE_PM_BATTERY
	/**
	 * @brief Battery manager instance
	 */
	BatteryManager & bm;
	/**
	 * @brief Battery object instance
	 */
	BatteryPtr_t pbatt;

	/**
	 * @struct SystemLifetimeInfo_t
	 * @brief System power budget information
	 */
	struct SystemLifetimeInfo_t {
		/** Mutex to protect concurrent accesses */
		std::mutex mtx;
		/** Time point of the required system lifetime */
		std::chrono::system_clock::time_point target_time;
		/** System power budget for guarateeing the required lifetime */
		int32_t power_budget_mw = 0;
		/** If true the request is to keep the system always on */
		bool always_on;
	} sys_lifetime;

#endif // CONFIG_BBQUE_PM_BATTERY

	/**
	 * @brief Command manager instance
	 */
	CommandManager & cm;

#ifdef CONFIG_BBQUE_DM
	/**
	 * @brief Data manager instance
	 */
	DataManager & dm;
#endif

	/**
	 * @brief Configuration manager instance
	 */
	ConfigurationManager & cfm;

	/**
	 * @brief The logger used by the power manager
	 */
	std::unique_ptr<bu::Logger> logger;

	/**
	 * @brief The set of flags related to pending monitoring events to handle
	 */
	std::bitset<WM_EVENT_COUNT> events;

	/**
	 * @struct ResourceHandler
	 */
	struct ResourceHandler {
		br::ResourcePathPtr_t path;
		br::ResourcePtr_t resource_ptr;
	};


	/**
	 * @struct PowerMonitorInfo_t
	 * @brief Structure to collect support information for the power
	 * monitoring activity
	 */
	struct PowerMonitorInfo_t {
		// Resource handlers
		std::vector<ResourceHandler> resources;   /** Resources to monitor */
		// Data logging
		std::map<br::ResourcePathPtr_t, std::ofstream *> log_fp; /** Output file descriptors  */
		std::string log_dir;       /** Output file directory    */
		bool log_enabled = false;  /** Enable / disable         */
		// Monitoring status
		bool started = false;      /** Monitoring start/stop            */
		uint32_t period_ms;        /** Monitoring period (milliseconds) */
	} wm_info;


	/**
	 * @brief Number of monitoring threads to spawn
	 */
	uint16_t nr_threads = 1;

	/**
	 * @brief Keep track of sending status of an optimization request
	 */
	std::atomic<bool> opt_request_sent;

	/**
	 * @brief Track it an optimization request is due to the battery charge level
	 */
	bool opt_request_for_battery = false;


	/**
	 * @struct Data to manage the triggers execution
	 */
	//struct TriggerInfo_t {
	//	std::shared_ptr<bbque::trig::Trigger> obj;   /** Trigger object to call */
	//	uint32_t threshold_high = 0;                      /** Threshold high value */
	//	uint32_t threshold_low = 0;                  /** Threshold low armed value */
	//	float margin       = 0.1;                    /** Margin [0..1) */
	//};

	/**
	 * @brief Threshold values for triggering an optimization request
	 */
	//std::map<PowerManager::InfoType, TriggerInfo_t> triggers;
	std::map<PowerManager::InfoType, std::shared_ptr<bbque::trig::Trigger>> triggers;

	/**
	 * @brief Deferrable for coalescing multiple optimization requests
	 **/
	bbque::utils::Deferrable optimize_dfr;


	/** Function pointer to PowerManager member functions */
	using PMfunc = std::function<
		PowerManager::PMResult(PowerManager&, br::ResourcePathPtr_t const &, uint32_t &)>;

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
	 *
	 * @param first_resource_index First resource to monitor (from reg. index)
	 * @param last_resource_index Last resource to monitor (from reg. array)
	 */
	void SampleResourcesStatus(uint16_t first_resource_index, uint16_t last_resource_index);

	/**
	 * @brief Periodic task
	 */
	virtual void Task();

	void BuildLogString(
			br::ResourcePtr_t rsrsc,
			uint info_idx,
			std::string & inst_values,
			std::string & mean_values);

	/**
	 * @brief Log a data text line to file
	 *
	 * @param Resource path
	 * @param line Line to dump
	 * @param om C++ stream open mode
	 */
	void DataLogWrite(
			br::ResourcePathPtr_t rp,
			std::string const & data_line,
			std::ios_base::openmode om = std::ios::out | std::ios_base::app);

	/**
	 * @brief Clear data log files
	 */
	void DataLogClear();

	/**
	 * @brief Manage the data log on file behavior
	 *
	 * @param arg A string containing the action to perform (start, stop,
	 * clear).
	 */
	int DataLogCmdHandler(const char * arg);


	/**
	 * @brief Manage a trigger and conditionally send an optimization request
	 * @param info_type Type of runtime information
	 * @param curr_value Current value (e.g. of temperature)
	 */
	void ManageRequest(PowerManager::InfoType info_type, double curr_value);

	/**
	 * @brief Trigger execution: check if the current monitored value worth an
	 * optimization policy execution request
	 */
	inline void ExecuteTrigger(br::ResourcePtr_t rsrc, PowerManager::InfoType info_type) {
			ManageRequest(info_type, rsrc->GetPowerInfo(info_type, br::Resource::MEAN));
	}

	/**
	 * @brief Send an optimization request to execute the resource allocation policy
	 */
	void SendOptimizationRequest();


#ifdef CONFIG_BBQUE_PM_BATTERY
	/**
	 * @brief Sample the battery status
	 */
	void SampleBatteryStatus();

	/**
	 * @brief Compute the system power budget
	 * @return The power value in milliwatts.
	 */
	inline uint32_t ComputeSysPowerBudget() const {
		// How many seconds remains before lifetime target is reached?
		std::chrono::seconds secs_from_now = GetSysLifetimeLeft();
		// System energy budget in mJ
		uint32_t energy_budget = pbatt->GetChargeMAh() * 3600 *
					pbatt->GetVoltage() / 1e3;
		return energy_budget / secs_from_now.count();
	}

	/**
	 * @brief Trigger execution for the battery status.
	 * The optimization is required in case of battery level under
	 * a given threshold
	 */
	void ExecuteTriggerForBattery();

	/**
	 * @brief System target lifetime setting
	 * @param action The control actions:
	 *		set   (to set the amount of hours)
	 *		info  (to get the current information)
	 *		clear (to clear the target)
	 *		help  (command help)
	 * @param hours For the action 'set' only
	 * @return 0 for success, a negative number in case of error
	 */
	int SystemLifetimeCmdHandler(
			const std::string action,
			const std::string arg);

	/**
	 * @brief System target lifetime information report
	 */
	void PrintSystemLifetimeInfo() const;

#endif // CONFIG_BBQUE_PM_BATTERY

};

} // namespace bbque

#endif // BBQUE_POWER_MONITOR_H_

