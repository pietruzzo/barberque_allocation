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


#define MODULE_NAMESPACE POWER_MANAGER_NAMESPACE

namespace bbque {


PowerManager & PowerManager::GetInstance() {
	static PowerManager instance;
	return instance;
}


PowerManager::PowerManager() {

	// Get a logger module
	logger = bu::Logger::GetLogger(POWER_MANAGER_NAMESPACE);
	assert(logger);

	logger->Info("Initialize PowerManager...");

}


PowerManager::~PowerManager() {

}


PowerManager::PMResult PowerManager::GetLoad(
		ResourcePathPtr_t const & rp, uint32_t &perc) {
	(void)perc;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetLoad not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult PowerManager::GetTemperature(
		ResourcePathPtr_t const & rp, uint32_t &celsius) {
	(void)celsius;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetTemperature not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetClockFrequency(ResourcePathPtr_t const & rp, uint32_t &khz) {
	(void)khz;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetClockFrequency not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetClockFrequencyInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step) {
	(void)khz_min;
	(void)khz_max;
	(void)khz_step;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetClockFrequencyInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetClockFrequency(ResourcePathPtr_t const & rp, uint32_t khz) {
	(void)rp;
	(void)khz;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) SetClockFrequency not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetVoltage(ResourcePathPtr_t const & rp, uint32_t &volt) {
	(void)volt;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetVoltage not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetVoltageInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &volt_min,
		uint32_t &volt_max,
		uint32_t &volt_step) {
	(void)volt_min;
	(void)volt_max;
	(void)volt_step;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetVoltageInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value) {
	(void)fs_type;
	(void)value;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetFanSpeed not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetFanSpeedInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step) {
	(void)rpm_min;
	(void)rpm_max;
	(void)rpm_step;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetFanSpeedInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value) {
	(void)fs_type;
	(void)value;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) SetFanSpeed not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::ResetFanSpeed(ResourcePathPtr_t const & rp) {
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) ResetFanSpeed not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetPowerUsage(ResourcePathPtr_t const & rp, uint32_t &mwatt) {
	(void)mwatt;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetPowerUsage not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &mwatt_min,
		uint32_t &mwatt_max) {
	(void)mwatt_min;
	(void)mwatt_max;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetPowerInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerState(ResourcePathPtr_t const & rp, int &state) {
	(void)state;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetPowerState not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPowerStatesInfo(
	ResourcePathPtr_t const & rp,
	int & min, int & max, int & step) {
	(void)min;
	(void)max;
	(void)step;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetPowerStatesInfo not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetPowerState(ResourcePathPtr_t const & rp, int state) {
	(void)state;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) SetPowerState not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}


PowerManager::PMResult
PowerManager::GetPerformanceState(ResourcePathPtr_t const & rp, int &state) {
	(void)state;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetPerformanceState not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::GetPerformanceStatesCount(ResourcePathPtr_t const & rp, int &count) {
	(void)count;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) GetPerformanceStatesCount not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

PowerManager::PMResult
PowerManager::SetPerformanceState(ResourcePathPtr_t const & rp, int state) {
	(void)state;
	switch (rp->Type()) {
	default:
		break;
	}
	logger->Warn("(PM) SetPerformanceState not supported for [%s]",
			br::ResourceIdentifier::TypeStr[rp->Type()]);
	return PMResult::ERR_API_NOT_SUPPORTED;
}

} // namespace bbque

