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
#include "bbque/config.h"

#ifdef CONFIG_BBQUE_PM_AMD
# include "bbque/pm/power_manager_amd.h"
#endif

#define MODULE_NAMESPACE POWER_MANAGER_NAMESPACE

namespace bbque {


PowerManager & PowerManager::GetInstance() {
	static PowerManager instance;
	return instance;
}


PowerManager::PowerManager() {
	static bool initialized = false;
	cm = &(CommandManager::GetInstance());

	// Get a logger module
	logger = bu::Logger::GetLogger(POWER_MANAGER_NAMESPACE);
	assert(logger);

	// Block recursion on vendor specific construction
	if (initialized) return;
	initialized = true;

	logger->Info("Initialize PowerManager...");

#ifdef CONFIG_BBQUE_PM_AMD
	// Initialize the AMD GPU power manager
	logger->Notice("Using AMD provider for GPUs power management");
	gpu = std::unique_ptr<PowerManager>(new AMDPowerManager());
#endif // CONFIG_BBQUE_PM_AMD

}


PowerManager::~PowerManager() {

}


PowerManager::PMResult PowerManager::GetLoad(
		ResourcePathPtr_t const & rp, uint32_t &perc) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetLoad(rp, perc);
	default:
		break;
	}
	logger->Warn("(PM) GetLoad not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult PowerManager::GetTemperature(
		ResourcePathPtr_t const & rp, uint32_t &celsius) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetTemperature(rp, celsius);
	default:
		break;
	}
	logger->Warn("(PM) GetTemperature not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetClockFrequency(ResourcePathPtr_t const & rp, uint32_t &khz) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetClockFrequency(rp, khz);
	default:
		break;
	}
	logger->Warn("(PM) GetClockFrequency not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetClockFrequencyInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetClockFrequencyInfo(rp, khz_min, khz_max, khz_step);
	default:
		break;
	}
	logger->Warn("(PM) GetClockFrequencyInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetClockFrequency(ResourcePathPtr_t const & rp, uint32_t khz) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->SetClockFrequency(rp, khz);
	default:
		break;
	}
	logger->Warn("(PM) SetClockFrequency not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetVoltage(ResourcePathPtr_t const & rp, uint32_t &volt) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetVoltage(rp, volt);
	default:
		break;
	}
	logger->Warn("(PM) GetVoltage not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetVoltageInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &volt_min,
		uint32_t &volt_max,
		uint32_t &volt_step) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetVoltageInfo(rp, volt_min, volt_max, volt_step);
	default:
		break;
	}
	logger->Warn("(PM) GetVoltageInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetFanSpeed(rp, fs_type, value);
	default:
		break;
	}
	logger->Warn("(PM) GetFanSpeed not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetFanSpeedInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetFanSpeedInfo(rp, rpm_min, rpm_max, rpm_step);
	default:
		break;
	}
	logger->Warn("(PM) GetFanSpeedInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->SetFanSpeed(rp, fs_type, value);
	default:
		break;
	}
	logger->Warn("(PM) SetFanSpeed not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::ResetFanSpeed(ResourcePathPtr_t const & rp) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->ResetFanSpeed(rp);
	default:
		break;
	}
	logger->Warn("(PM) ResetFanSpeed not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetPowerUsage(ResourcePathPtr_t const & rp, uint32_t &mwatt) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetPowerUsage(rp, mwatt);
	default:
		break;
	}
	logger->Warn("(PM) GetPowerUsage not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &mwatt_min,
		uint32_t &mwatt_max) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetPowerInfo(rp, mwatt_min, mwatt_max);
	default:
		break;
	}
	logger->Warn("(PM) GetPowerInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerState(ResourcePathPtr_t const & rp, int &state) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetPowerState(rp, state);
	default:
		break;
	}
	logger->Warn("(PM) GetPowerState not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerStatesInfo(
	ResourcePathPtr_t const & rp,
	int & min, int & max, int & step) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetPowerStatesInfo(rp, min, max, step);
	default:
		break;
	}
	logger->Warn("(PM) GetPowerStatesInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetPowerState(ResourcePathPtr_t const & rp, int state) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->SetPowerState(rp, state);
	default:
		break;
	}
	logger->Warn("(PM) SetPowerState not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetPerformanceState(ResourcePathPtr_t const & rp, int &state) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetPerformanceState(rp, state);
	default:
		break;
	}
	logger->Warn("(PM) GetPerformanceState not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPerformanceStatesCount(ResourcePathPtr_t const & rp, int &count) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->GetPerformanceStatesCount(rp, count);
	default:
		break;
	}
	logger->Warn("(PM) GetPerformanceStatesCount not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetPerformanceState(ResourcePathPtr_t const & rp, int state) {
	switch (rp->ParentType(rp->Type())) {
	case br::ResourceIdentifier::GPU:
		if (!gpu) break;
		return gpu->SetPerformanceState(rp, state);
	default:
		break;
	}
	logger->Warn("(PM) SetPerformanceState not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->ParentType(rp->Type())]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

int PowerManager::CommandsCb(int argc, char *argv[]) {
	uint8_t cmd_offset = ::strlen(MODULE_NAMESPACE);
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

	logger->Error("Unexpected command: %s", command_id);
	return 0;
}
} // namespace bbque

