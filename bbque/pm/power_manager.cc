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

#include "bbque/pm/power_manager.h"
#include "bbque/res/resource_path.h"
#include "bbque/config.h"

#ifdef CONFIG_BBQUE_PM_AMD
# include "bbque/pm/power_manager_amd.h"
#endif
#ifdef CONFIG_BBQUE_PM_GPU_ARM_MALI
# include "bbque/pm/power_manager_gpu_arm_mali.h"
#endif

#ifdef CONFIG_BBQUE_PM_CPU
# include "bbque/pm/power_manager_cpu.h"
#ifdef CONFIG_TARGET_ODROID_XU // Odroid XU-3
# include "bbque/pm/power_manager_cpu_odroidxu.h"
#endif // CONFIG_TARGET_ODROID_XU
#ifdef CONFIG_TARGET_ARM_CORTEX_A9
# include "bbque/pm/power_manager_cpu_arm_cortexa9.h"
#endif // CONFIG_TARGET_FREESCALE_IMX6Q
#endif


#define MODULE_NAMESPACE POWER_MANAGER_NAMESPACE

namespace bw = bbque::pm;

namespace bbque {

std::array<
	PowerManager::InfoType,
	int(PowerManager::InfoType::COUNT)> PowerManager::InfoTypeIndex = {{
		InfoType::LOAD        ,
		InfoType::TEMPERATURE ,
		InfoType::FREQUENCY   ,
		InfoType::POWER       ,
		InfoType::CURRENT     ,
		InfoType::VOLTAGE     ,
		InfoType::PERF_STATE  ,
		InfoType::POWER_STATE
	}};

std::array<
	const char *,
	int(PowerManager::InfoType::COUNT)> PowerManager::InfoTypeStr = {{
		"load"        ,
		"temperature" ,
		"frequency"   ,
		"power"       ,
		"current"     ,
		"voltage"     ,
		"performance state",
		"power state"
	}};

PowerManager & PowerManager::GetInstance() {
	static PowerManager instance;
	return instance;
}


PowerManager::PowerManager() {
	static bool initialized = false;
	cm = &(CommandManager::GetInstance());
	mm = &(bw::ModelManager::GetInstance());

	// Get a logger module
	logger = bu::Logger::GetLogger(POWER_MANAGER_NAMESPACE);
	assert(logger);

	// Block recursion on vendor specific construction
	if (initialized) return;
	logger->Info("Initialize PowerManager...");
	initialized = true;

	// Register command to set device fan speed
#define CMD_FANSPEED_SET "fanspeed_set"
	cm->RegisterCommand(MODULE_NAMESPACE "." CMD_FANSPEED_SET,
		static_cast<CommandHandler*>(this),
		"Set the speed of the fan (percentage value)");

	// GPU
#ifdef CONFIG_BBQUE_PM_AMD
	logger->Notice("Using AMD provider for GPUs power management");
	gpu = std::unique_ptr<PowerManager>(new AMDPowerManager());
#endif
#ifdef CONFIG_BBQUE_PM_GPU_ARM_MALI
	logger->Notice("Using ARM Mali provider for GPUs power management");
	gpu = std::unique_ptr<PowerManager>(new ARM_Mali_GPUPowerManager());
#endif

	// CPU
#ifdef CONFIG_BBQUE_PM_CPU
# ifdef CONFIG_TARGET_ODROID_XU
	logger->Notice("Using ODROID-XU CPU power management module");
	cpu = std::unique_ptr<PowerManager>(new ODROID_XU_CPUPowerManager());
	return;
# endif
# ifdef CONFIG_TARGET_ARM_CORTEX_A9
	logger->Notice("Using ARM Cortex A9 CPU power management module");
	cpu = std::unique_ptr<PowerManager>(new ARM_CortexA9_CPUPowerManager());
	return;
# endif
   // Generic
	logger->Notice("Using generic CPU power management module");
	cpu = std::unique_ptr<PowerManager>(new CPUPowerManager());
#endif // CONFIG_BBQUE_PM_CPU

}


PowerManager::~PowerManager() {

}


PowerManager::PMResult PowerManager::GetLoad(
		ResourcePathPtr_t const & rp, uint32_t &perc) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetLoad(rp, perc);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->GetLoad(rp, perc);
	default:
		break;
	}
	logger->Warn("(PM) GetLoad not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult PowerManager::GetTemperature(
		ResourcePathPtr_t const & rp, uint32_t &celsius) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetTemperature(rp, celsius);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->GetTemperature(rp, celsius);
	default:
		break;
	}
	logger->Warn("(PM) GetTemperature not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetClockFrequency(ResourcePathPtr_t const & rp, uint32_t &khz) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetClockFrequency(rp, khz);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->GetClockFrequency(rp, khz);
	default:
		break;
	}
	logger->Warn("(PM) GetClockFrequency not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetClockFrequencyInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetClockFrequencyInfo(rp, khz_min, khz_max, khz_step);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->GetClockFrequencyInfo(rp, khz_min, khz_max, khz_step);
	default:
		logger->Warn("(PM) GetClockFrequencyInfo not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	return PMResult::OK;
}

PowerManager::PMResult
PowerManager::GetAvailableFrequencies(
		ResourcePathPtr_t const & rp,
		std::vector<uint32_t> & freqs) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetAvailableFrequencies(rp, freqs);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->GetAvailableFrequencies(rp, freqs);
	default:
		break;
	}
	logger->Warn("(PM) GetAvailableFrequencies not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetClockFrequency(ResourcePathPtr_t const & rp, uint32_t khz) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->SetClockFrequency(rp, khz);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->SetClockFrequency(rp, khz);
	default:
		break;
	}
	logger->Warn("(PM) SetClockFrequency not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::SetClockFrequency(
		ResourcePathPtr_t const & rp,
		uint32_t min_khz, uint32_t max_khz) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->SetClockFrequency(rp, min_khz, max_khz);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->SetClockFrequency(rp, min_khz, max_khz);
	default:
		break;
	}
	logger->Warn("(PM) SetClockFrequency not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}


std::vector<std::string> const &
PowerManager::GetAvailableFrequencyGovernors(br::ResourcePathPtr_t const & rp) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetAvailableFrequencyGovernors(rp);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->GetAvailableFrequencyGovernors(rp);
	default:
		break;
	}
	logger->Warn("(PM) GetClockFrequencyGovernors not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return cpufreq_governors;
}

PowerManager::PMResult
PowerManager::GetClockFrequencyGovernor(
		br::ResourcePathPtr_t const & rp,
		std::string & governor) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetClockFrequencyGovernor(rp, governor);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->GetClockFrequencyGovernor(rp, governor);
	default:
		break;
	}
	logger->Warn("(PM) SetClockFrequencyGovernor not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetClockFrequencyGovernor(
		br::ResourcePathPtr_t const & rp,
		std::string const & governor) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->SetClockFrequencyGovernor(rp, governor);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->SetClockFrequencyGovernor(rp, governor);
	default:
		break;
	}
	logger->Warn("(PM) SetClockFrequencyGovernor not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetVoltage(ResourcePathPtr_t const & rp, uint32_t &volt) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetVoltage(rp, volt);
	default:
		break;
	}
	logger->Warn("(PM) GetVoltage not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetVoltageInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &volt_min,
		uint32_t &volt_max,
		uint32_t &volt_step) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetVoltageInfo(rp, volt_min, volt_max, volt_step);
	default:
		break;
	}
	logger->Warn("(PM) GetVoltageInfo not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetFanSpeed(rp, fs_type, value);
	default:
		break;
	}
	logger->Warn("(PM) GetFanSpeed not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetFanSpeedInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetFanSpeedInfo(rp, rpm_min, rpm_max, rpm_step);
	default:
		break;
	}
	logger->Warn("(PM) GetFanSpeedInfo not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->SetFanSpeed(rp, fs_type, value);
	default:
		break;
	}
	logger->Warn("(PM) SetFanSpeed not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::ResetFanSpeed(ResourcePathPtr_t const & rp) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->ResetFanSpeed(rp);
	default:
		break;
	}
	logger->Warn("(PM) ResetFanSpeed not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetPowerUsage(ResourcePathPtr_t const & rp, uint32_t &mwatt) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetPowerUsage(rp, mwatt);
	case br::ResourceType::CPU:
		if (!cpu) break;
		return cpu->GetPowerUsage(rp, mwatt);
	default:
		break;
	}
	logger->Warn("(PM) GetPowerUsage not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &mwatt_min,
		uint32_t &mwatt_max) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetPowerInfo(rp, mwatt_min, mwatt_max);
	default:
		break;
	}
	logger->Warn("(PM) GetPowerInfo not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerState(ResourcePathPtr_t const & rp, uint32_t &state) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetPowerState(rp, state);
	default:
		break;
	}
	logger->Warn("(PM) GetPowerState not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerStatesInfo(
	ResourcePathPtr_t const & rp,
	uint32_t & min, uint32_t & max, int & step) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetPowerStatesInfo(rp, min, max, step);
	default:
		break;
	}
	logger->Warn("(PM) GetPowerStatesInfo not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetPowerState(ResourcePathPtr_t const & rp, uint32_t state) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->SetPowerState(rp, state);
	default:
		break;
	}
	logger->Warn("(PM) SetPowerState not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetPerformanceState(ResourcePathPtr_t const & rp, uint32_t &state) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetPerformanceState(rp, state);
	default:
		break;
	}
	logger->Warn("(PM) GetPerformanceState not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPerformanceStatesCount(ResourcePathPtr_t const & rp, uint32_t &count) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->GetPerformanceStatesCount(rp, count);
	default:
		break;
	}
	logger->Warn("(PM) GetPerformanceStatesCount not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetPerformanceState(ResourcePathPtr_t const & rp, uint32_t state) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceType::GPU:
		if (!gpu) break;
		return gpu->SetPerformanceState(rp, state);
	default:
		break;
	}
	logger->Warn("(PM) SetPerformanceState not supported for [%s]",
			br::GetResourceTypeString(rp->ParentType(rp->Type())));
	return PMResult::ERR_API_NOT_SUPPORTED;
}

int PowerManager::CommandsCb(int argc, char *argv[]) {
	uint8_t cmd_offset = ::strlen(MODULE_NAMESPACE) + 1;
	char * command_id  = argv[0] + cmd_offset;

	logger->Info("Processing command [%s]", command_id);
	if (argc < 2) {
		logger->Error("(PM) Invalid command format");
		return 1;
	}

	br::ResourcePathPtr_t rp(new br::ResourcePath(argv[1]));
	if (rp == nullptr) {
		logger->Error("(PM) Please specify a valid resource path");
		return 2;
	}

	// Set fan speed
	if (!strncmp(CMD_FANSPEED_SET, command_id, strlen(CMD_FANSPEED_SET))) {
		if (argc != 3) {
			logger->Error("(PM) Usage: "
				"'%s.%s  sys[0-9].gpu[0-9] [0..100]' ",
				MODULE_NAMESPACE, CMD_FANSPEED_SET);
			return 3;
		}

		uint8_t speed_perc = atoi(argv[2]);
		return FanSpeedSetHandler(rp, speed_perc);
	}

	logger->Error("Unexpected command: %s", command_id);
	return 0;
}

int PowerManager::FanSpeedSetHandler(
		br::ResourcePathPtr_t const & rp,
		uint8_t speed_perc) {
	if (speed_perc > 100) {
		logger->Error("(PM) Invalid value (%d). Expected [0..100]");
		return 4;
	}
	switch (rp->Type()) {
	case br::ResourceType::GPU:
		if (!gpu)  {
			logger->Warn("(PM) No power manager available for GPU(s)");
			return 4;
		}
		logger->Notice("Setting fan speed of %s to %d %",
				rp->ToString().c_str(), speed_perc);
		gpu->SetFanSpeed(rp, FanSpeedType::PERCENT, speed_perc);
		break;
	default:
		return 5;
	}

	return 0;
}

} // namespace bbque

