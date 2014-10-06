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

#include "bbque/pm/power_manager_cpu.h"

#include <fstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#define LOAD_SAMPLING_INTERVAL_SECONDS 1
#define LOAD_SAMPLING_NUMBER 1

#define PROCSTAT_FIRST 1
#define PROCSTAT_LAST 10

#define PROCSTAT_IDLE 4
#define PROCSTAT_IOWAIT 5

namespace bbque {

CPUPowerManager::CPUPowerManager() {
	int cpu_id  = -1;
	int core_id = 0;
	std::string core_string;

	while (++cpu_id >= 0) {
		std::ifstream cpu_info(
			"/sys/devices/system/cpu/cpu" + std::to_string(cpu_id) +
			"/topology/core_id");
		if (!cpu_info) break;

		// If the CPU exists, extract the core_id and save the pair
		std::getline(cpu_info, core_string);
		core_id = atoi(core_string.c_str());
		core_ids[cpu_id] = core_id;
	}
}

CPUPowerManager::ExitStatus CPUPowerManager::GetLoadInfo(
		CPUPowerManager::LoadInfo * info,
		std::string cpu_core_id) {
	// Information about kernel activity is available in the /proc/stat
	// file. All the values are aggregated since the system first booted.
	// Thus, to compute the load, the variation of these values in a little
	//  and constant timespan has to be computed.
	std::ifstream procstat_info("/proc/stat");
	std::string   procstat_entry;

	// The information about CPU-N can be found in the line whose sintax
	// follows the pattern:
	// 	cpun x y z w ...
	// Check the Linux documentation to find information about those values
	boost::regex cpu_info_stats("cpu" + cpu_core_id +
				" (\\d+) (\\d+) (\\d+) (\\d+) (\\d+) (\\d+)" +
				" (\\d+) (\\d+) (\\d+) (\\d+)");

	// Parsing the /proc/stat file to find the correct line
	bool found = false;
	while (std::getline(procstat_info, procstat_entry)) {
		boost::smatch match;
		// If that's the correct line
		if (!boost::regex_match(procstat_entry, match, cpu_info_stats))
			continue;
		found = true;

		// CPU core total time
		for (int i = PROCSTAT_FIRST; i <= PROCSTAT_LAST; ++i)
			info->total += boost::lexical_cast<int32_t>(match[i]);
		// CPU core idle time
		for (int i = PROCSTAT_IDLE; i <= PROCSTAT_IOWAIT; ++i)
			info->idle += boost::lexical_cast<int32_t>(match[i]);
	}

	if (!found) return CPUPowerManager::ExitStatus::ERR_GENERIC;
	return CPUPowerManager::ExitStatus::OK;
}

CPUPowerManager::~CPUPowerManager() {
	core_ids.clear();
}

PowerManager::PMResult CPUPowerManager::GetLoad(
		ResourcePathPtr_t const & rp,
		uint32_t & perc){
	CPUPowerManager::ExitStatus result;
	// Getting the load of a specified CPU. This is possible by reading the
	// /proc/stat file exposed by Linux. To compute the load, a minimum of two
	// samples are needed. In fact, the measure is obtained computing the
	// variation of the file content between two consecutive accesses.
	CPUPowerManager::LoadInfo start_info, end_info;

	// Extracting the selected CPU from the resource path. -1 if error
	int cpu_logic_id = GetCPU(rp);
	if (cpu_logic_id < 0)
		return PowerManager::PMResult::ERR_RSRC_INVALID_PATH;
	std::string cpu_id = std::to_string(cpu_logic_id);

	for (int i = 0; i < LOAD_SAMPLING_NUMBER; ++i) {
		// Performing the actual accesses, with an interval of
		// LOAD_SAMPLING_INTERVAL_SECONDS (circa)
		result = GetLoadInfo(&start_info, cpu_id);
		if (result != ExitStatus::OK) {
			logger->Error("No activity info on CPU core %d", cpu_id.c_str());
			return PMResult::ERR_INFO_NOT_SUPPORTED;
		}

		sleep(LOAD_SAMPLING_INTERVAL_SECONDS);
		result = GetLoadInfo(&end_info, cpu_id);
		if (result != ExitStatus::OK) {
			logger->Error("No activity info on CPU core %d", cpu_id.c_str());
			return PMResult::ERR_INFO_NOT_SUPPORTED;
		}

		// Usage is computed as 1 - idle_time[%]
		float usage =
			100 - (100 * (float)(end_info.idle - start_info.idle) /
			(float)(end_info.total - start_info.total));
		// If the usage is very low and LOAD_SAMPLING_INTERVAL_SECONDS
		// is very little, the usage could become negative, because
		// both the computing and the contents of /proc/stat are not
		// so accurate
		if (usage < 0) usage = 0;
		perc = static_cast<uint32_t>(usage);
	}

	return PowerManager::PMResult::OK;
}

int CPUPowerManager::GetCPU(ResourcePathPtr_t const & rp){
	// If the CPUPowerManager has been called, the path refers to a CPU.
	// Thus, further security checks on the string are not needed.
	boost::regex path_structure(".{1,}\\.pe(\\d+)");
	boost::smatch match;
	std::string str_path(rp->ToString());

	if (! boost::regex_match(str_path, match, path_structure)) {
		logger->Warn("Resource path parsing error.");
		return -1;
	}

	int logic_id = boost::lexical_cast<int>(match[1]);
	return logic_id;
}

PowerManager::PMResult CPUPowerManager::GetClockFrequency(
	ResourcePathPtr_t const & rp, uint32_t &khz){

	// Extracting the PE id from the resource path
	int cpu_logic_id = GetCPU(rp);
	if (cpu_logic_id < 0) {
		logger->Warn("Frequency value not available for %s",
			rp->ToString().c_str());
		return PowerManager::PMResult::ERR_RSRC_INVALID_PATH;
	}

	// Getting the frequency value
	khz = (uint32_t)cpufreq_get_freq_hardware((unsigned int)cpu_logic_id);

	return PowerManager::PMResult::OK;

}

} // namespace bbque
