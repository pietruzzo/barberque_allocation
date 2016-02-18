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

#include "bbque/resource_accounter.h"
#include "bbque/pm/power_manager_cpu.h"
#include "bbque/res/resource_path.h"
#include "bbque/utils/iofs.h"

#include <fstream>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#define LOAD_SAMPLING_INTERVAL_SECONDS 1
#define LOAD_SAMPLING_NUMBER 1

#define PROCSTAT_FIRST  1
#define PROCSTAT_LAST   10

#define PROCSTAT_IDLE   4
#define PROCSTAT_IOWAIT 5

#define TEMP_SENSOR_FIRST_ID 1
#define TEMP_SENSOR_STEP_ID  1

namespace bu = bbque::utils;

namespace bbque {

CPUPowerManager::CPUPowerManager():
		prefix_sys_cpu(BBQUE_LINUX_SYS_CPU_PREFIX) {
	bu::IoFs::ExitCode_t result;
	int cpu_id  = -1;
	int core_id = 0;
	// CPU <--> Core id mapping:
	// CPU is commonly used to reference the cores while in the BarbequeRTRM
	// 'core' is referenced as 'processing element', thus it needs a unique id
	// number. For instance in a SMT Intel 4-core:
	// -----------------------------------------------------------------------
	// CPU/HWt  Cores
	// 0        0
	// 1        0
	// 2        1
	// 3        1
	// ------------------------------------------------------------------------
	// Therefore we consider 'processing element' what Linux calls CPU
	//-------------------------------------------------------------------------
	while (++cpu_id) {
		result = bu::IoFs::ReadIntValueFrom<int>(
				BBQUE_LINUX_SYS_CPU_PREFIX + std::to_string(cpu_id) +
				"/topology/core_id",
				core_id);
		if (result != bu::IoFs::OK) break;

		// Processing element (cpu_id) / core_id
		core_ids[cpu_id]   = core_id;
		// Available frequencies per core
		core_freqs[cpu_id] = _GetAvailableFrequencies(cpu_id);
	}

	// Thermal sensors mapping
	char str_value[8];
	int sensor_id = TEMP_SENSOR_FIRST_ID;
	while (1) {
		std::string therm_file(
				BBQUE_LINUX_SYS_CPU_THERMAL + std::to_string(sensor_id) +
				"_label");
		result = bu::IoFs::ReadValueFrom(therm_file.c_str(), str_value, 8);
		if (result != bu::IoFs::OK) {
			logger->Warn("Failed in reading from %s", therm_file.c_str());
			break;
		}

		// Look for the label containing the core ID required
		std::string core_label(str_value);
		sensor_id += TEMP_SENSOR_STEP_ID;
		if (core_label.find("Core") != 0) {
			logger->Crit("Label = %s", core_label.c_str());
			continue;
		}

		int core_id = std::stoi(core_label.substr(5));
		core_therms[core_id] = new std::string(
				BBQUE_LINUX_SYS_CPU_THERMAL + std::to_string(sensor_id) +
				"_input");
		logger->Crit("Thermal sensors: PE (core) %d: %s", core_id,
				therm_file.c_str());
	}

	// CPUfreq governors
	char govs[100];
	memset(govs, 0, sizeof(govs)-1);
	std::string cpufreq_path(prefix_sys_cpu +
			"0/cpufreq/scaling_available_governors");
	result = bu::IoFs::ReadValueFrom(cpufreq_path, govs, 100);
	if (result != bu::IoFs::OK) {
		logger->Error("Error reading: %s", cpufreq_path.c_str());
		return;
	}
	logger->Info("CPUfreq governors: ");
	std::string govs_str(govs);
	while (govs_str.size() > 1)
		cpufreq_governors.push_back(
				br::ResourcePathUtils::SplitAndPop(govs_str, " "));
	for (std::string & g: cpufreq_governors)
		logger->Info("---> %s", g.c_str());
}


CPUPowerManager::ExitStatus CPUPowerManager::GetLoadInfo(
		CPUPowerManager::LoadInfo * info,
		br::ResID_t cpu_core_id) const {
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
	boost::regex cpu_info_stats("cpu" + std::to_string(cpu_core_id) +
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
	core_therms.clear();
	core_freqs.clear();
	cpufreq_governors.clear();
}

PowerManager::PMResult CPUPowerManager::GetLoad(
		ResourcePathPtr_t const & rp,
		uint32_t & perc){
	PMResult result;
	ResourceAccounter & ra(ResourceAccounter::GetInstance());

	// Extract the single CPU core (PE) id from the resource path
	// (e.g., "cpu2.pe3", pe_id = 3)
	br::ResID_t pe_id = rp->GetID(br::ResourceIdentifier::PROC_ELEMENT);
	if (pe_id >= 0) {
		result = GetLoadCPU(pe_id, perc);
		if (result != PMResult::OK) return result;
	}
	else {
		// Multiple CPU cores (e.g., "cpu2.pe")
		uint32_t pe_load = 0;
		perc = 0;

		// Cumulate the load of each core
		br::ResourcePtrList_t const & r_list(ra.GetResources(rp));
		for (ResourcePtr_t rsrc: r_list) {
			result = GetLoadCPU(rsrc->ID(), pe_load);
			if (result != PMResult::OK) return result;
			perc += pe_load;
		}

		// Return the average
		perc /= r_list.size();
	}
	return PowerManager::PMResult::OK;
}


std::vector<unsigned long> * CPUPowerManager::_GetAvailableFrequencies(int cpu_id) {
	bu::IoFs::ExitCode_t result;

	// Extracting available frequencies
	char cpu_available_freqs[100];
	result = bu::IoFs::ReadValueFrom(
				BBQUE_LINUX_SYS_CPU_PREFIX + std::to_string(cpu_id) +
				"/cpufreq/scaling_available_frequencies",
				cpu_available_freqs, 100);
	if (result != bu::IoFs::OK) {
		logger->Warn("List of frequencies not available for cpu %d", cpu_id);
		return nullptr;
	}

	// Storing the frequencies values in the vector
	std::vector<unsigned long>  * cpu_freqs =
		new  std::vector<unsigned long>();
	char * freq = strtok(cpu_available_freqs, " ");
	while (freq != nullptr) {
		cpu_freqs->push_back(std::stoi(freq));
		strtok(cpu_available_freqs, " ");
	}
	return cpu_freqs;
}

PowerManager::PMResult CPUPowerManager::GetLoadCPU(
		ResID_t cpu_core_id,
		uint32_t & load) const {
	CPUPowerManager::ExitStatus result;
	CPUPowerManager::LoadInfo start_info, end_info;

	// Getting the load of a specified CPU. This is possible by reading the
	// /proc/stat file exposed by Linux. To compute the load, a minimum of two
	// samples are needed. In fact, the measure is obtained computing the
	// variation of the file content between two consecutive accesses.
	for (int i = 0; i < LOAD_SAMPLING_NUMBER; ++i) {
		// Performing the actual accesses, with an interval of
		// LOAD_SAMPLING_INTERVAL_SECONDS (circa)
		result = GetLoadInfo(&start_info, cpu_core_id);
		if (result != ExitStatus::OK) {
			logger->Error("No activity info on CPU core %d", cpu_core_id);
			return PMResult::ERR_INFO_NOT_SUPPORTED;
		}

		sleep(LOAD_SAMPLING_INTERVAL_SECONDS);
		result = GetLoadInfo(&end_info, cpu_core_id);
		if (result != ExitStatus::OK) {
			logger->Error("No activity info on CPU core %d", cpu_core_id);
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
		load = static_cast<uint32_t>(usage);
	}

	return PowerManager::PMResult::OK;
}


PowerManager::PMResult CPUPowerManager::GetClockFrequency(
		ResourcePathPtr_t const & rp,
		uint32_t & khz){
	bu::IoFs::ExitCode_t result;

	// Extracting the PE id from the resource path
	int pe_id = rp->GetID(br::Resource::PROC_ELEMENT);
	if (pe_id < 0) {
		logger->Warn("Frequency value not available for %s",
				rp->ToString().c_str());
		return PowerManager::PMResult::ERR_RSRC_INVALID_PATH;
	}

	// Getting the frequency value
	result = bu::IoFs::ReadIntValueFrom<uint32_t>(
				BBQUE_LINUX_SYS_CPU_PREFIX + std::to_string(pe_id) +
				"/cpufreq/scaling_cur_freq",
				khz);
	if (result != bu::IoFs::OK) {
		logger->Warn("Cannot read current frequency for %s",
				rp->ToString().c_str());
		return PMResult::ERR_SENSORS_ERROR;
	}

	return PowerManager::PMResult::OK;
}

PowerManager::PMResult CPUPowerManager::GetTemperature(
		ResourcePathPtr_t const & rp,
		uint32_t & celsius){
	PMResult result = PMResult::ERR_INFO_NOT_SUPPORTED;
	bu::IoFs::ExitCode_t io_result;
	celsius = 0;

	int pe_id   = rp->GetID(br::Resource::PROC_ELEMENT);
	// We may have the same sensor for more than one processing element, the
	// sensor is referenced at "core" level
	int core_id = core_ids[pe_id];
	if (nullptr == core_therms[core_id]) {
		logger->Debug("Thermal sensor: not available");
		return result;
	}

	io_result =	bu::IoFs::ReadIntValueFrom<uint32_t>(
				core_therms[core_id]->c_str(), celsius);
	if (io_result != bu::IoFs::OK) {
		logger->Error("Cannot read current temperature for %s",
				rp->ToString().c_str());
		return PMResult::ERR_SENSORS_ERROR;
	}
	logger->Debug("Thermal sensor [%s] = %d", rp->ToString().c_str(), celsius);
	return PMResult::OK;
}

PowerManager::PMResult CPUPowerManager::GetClockFrequencyInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step) {
	// Extracting the PE id from the resource path
	int pe_id = rp->GetID(br::Resource::PROC_ELEMENT);
	if (pe_id < 0) {
		logger->Warn("Frequency info not available for %s",
				rp->ToString().c_str());
		return PowerManager::PMResult::ERR_RSRC_INVALID_PATH;
	}

	// Max and min frequency values
	auto edges = std::minmax_element(
			core_freqs[pe_id]->begin(), core_freqs[pe_id]->end());
	khz_min  = edges.first  - core_freqs[pe_id]->begin();
	khz_max  = edges.second - core_freqs[pe_id]->begin();
	// '0' to represent not fixed step value
	khz_step = 0;

	return PMResult::OK;
}

PowerManager::PMResult CPUPowerManager::GetAvailableFrequencies(
		ResourcePathPtr_t const & rp,
		std::vector<unsigned long> & freqs) {

	// Extracting the selected CPU from the resource path. -1 if error
	int pe_id = rp->GetID(br::Resource::PROC_ELEMENT);
	if (pe_id < 0)
		return PowerManager::PMResult::ERR_RSRC_INVALID_PATH;

	// Extracting available frequencies
	if (core_freqs[pe_id] == nullptr) {
		logger->Warn("List of frequencies not available for %s",
				rp->ToString().c_str());
	}
	freqs = *(core_freqs[pe_id]);

	return PowerManager::PMResult::OK;
}


PowerManager::PMResult CPUPowerManager::GetClockFrequencyGovernor(
		br::ResourcePathPtr_t const & rp,
		std::string & governor) {
	bu::IoFs::ExitCode_t result;
	char gov[12];
	int cpu_id = rp->GetID(br::Resource::PROC_ELEMENT);
	std::string cpufreq_path(prefix_sys_cpu + std::to_string(cpu_id) +
			"/cpufreq/scaling_governor");

	result = bu::IoFs::ReadValueFrom(cpufreq_path, gov, 12);
	if (result != bu::IoFs::ExitCode_t::OK)
		return PowerManager::PMResult::ERR_RSRC_INVALID_PATH;

	governor.assign(gov);
	return PowerManager::PMResult::OK;

}

PowerManager::PMResult CPUPowerManager::SetClockFrequencyGovernor(
		br::ResourcePathPtr_t const & rp,
		std::string const & governor) {
	bu::IoFs::ExitCode_t result;
	int cpu_id = rp->GetID(br::Resource::PROC_ELEMENT);
	std::string cpufreq_path(prefix_sys_cpu + std::to_string(cpu_id) +
			"/cpufreq/scaling_governor");

	result = bu::IoFs::WriteValueTo<std::string>(cpufreq_path, governor);
	logger->Debug("SetGovernor: %s", cpufreq_path.c_str());
	if (result != bu::IoFs::ExitCode_t::OK)
		return PowerManager::PMResult::ERR_RSRC_INVALID_PATH;

	return PowerManager::PMResult::OK;
}

} // namespace bbque
