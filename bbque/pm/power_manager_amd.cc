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

#include <dlfcn.h>

#include "bbque/pm/power_manager_amd.h"


#define  GET_PLATFORM_ADAPTER_ID(rp, adapter_id) \
	int adapter_id = GetAdapterId(rp); \
	if (adapter_id < 0) { \
		logger->Error("ADL: The path does not resolve a resource"); \
		return PMResult::ERR_RSRC_INVALID_PATH; \
	}

#define CHECK_OD_VERSION(adapter_id)\
	if (od_status_map[adapter_id].version != ADL_OD_VERSION) {\
		logger->Warn("ADL: Overdrive version %d not supported."\
			"Version %d expected", ADL_OD_VERSION); \
		return PMResult::ERR_API_VERSION;\
	}


namespace bbque {


/**   AMD Display Library */

void* __stdcall ADL_Main_Memory_Alloc ( int iSize )
{
    void* lpBuffer = malloc ( iSize );
    return lpBuffer;
}

void __stdcall ADL_Main_Memory_Free ( void** lpBuffer )
{
    if ( NULL != *lpBuffer )
    {
        free ( *lpBuffer );
        *lpBuffer = NULL;
    }
}

AMDPowerManager::AMDPowerManager() {
	// Retrieve information about the GPU(s) of the system
	LoadAdaptersInfo();
}

void AMDPowerManager::LoadAdaptersInfo() {
	int ADL_Err = ADL_ERR;
//	int status;
	int power_caps = 0;
	ODStatus_t od_status;
	ADLODParameters od_params;

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return;
	}

	// Adapters enumeration
	ADL_Err = ADL2_Adapter_NumberOfAdapters_Get(context, &adapters_count);
	if (adapters_count < 1) {
		logger->Error("ADL: No adapters available on the system");
		return;
	}
	logger->Info("ADL: Adapters count = %d", adapters_count);

	for (int i = 0; i < adapters_count; ++i) {
//		ADL_Err = ADL2_Adapter_Active_Get(context, i, &status);
//		if (ADL_Err != ADL_OK || status == ADL_FALSE) {
//			logger->Warn("Skipping '%d' [Err:%d] ", i, ADL_Err);
//			continue;
//		}
		// Adapters ID mapping and resouce path
		adapters_map.insert(
			std::pair<br::ResID_t, int>(adapters_map.size(), i));
		activity_map.insert(
			std::pair<int, ActivityPtr_t>(
				i, ActivityPtr_t(new ADLPMActivity)));

		// Power control capabilities
		ADL2_Overdrive5_PowerControl_Caps(context, i, &power_caps);
		power_caps_map.insert(std::pair<int, int>(i, power_caps));
		logger->Info("ADL: [A%d] Power control capabilities: %d",
			i, power_caps_map[i]);

		// Overdrive information
		ADL2_Overdrive_Caps(context, i,
			&od_status.supported, &od_status.enabled,
			&od_status.version);
		if (ADL_Err != ADL_OK) {
			logger->Error("ADL: Overdrive information read failed [%d]",
				ADL_Err);
			return;
		}
		od_status_map.insert(
			std::pair<int, ODStatus_t>(i, od_status));
		logger->Info("ADL: [A%d] Overdrive: "
			"[supported:%d, enabled:%d, version:%d]", i,
			od_status_map[i].supported,
			od_status_map[i].enabled,
			od_status_map[i].version);

		// Overdrive parameters
		ADL_Err = ADL2_Overdrive5_ODParameters_Get(context, i, &od_params);
		if (ADL_Err != ADL_OK) {
			logger->Error("ADL: Overdrive parameters read failed [%d]",
				ADL_Err);
			return;
		}
		od_params_map.insert(
			std::pair<int, ADLODParameters>(i, od_params));
		od_ready = true;
	}

	initialized = true;
	logger->Notice("ADL: Adapters [#=%d] information initialized",
		adapters_map.size());
	ADL2_Main_Control_Destroy(context);
}

AMDPowerManager::~AMDPowerManager() {
	for (auto adapter: adapters_map) {
		_ResetFanSpeed(adapter.second);
	}

	ADL2_Main_Control_Destroy(context);
	logger->Info("ADL: Control destroyed");

	adapters_map.clear();
	activity_map.clear();
	power_caps_map.clear();
	od_status_map.clear();
}

int AMDPowerManager::GetAdapterId(br::ResourcePathPtr_t const & rp) const {
	std::map<br::ResID_t, int>::const_iterator it;
	if (rp == nullptr) {
		logger->Error("ADL: Null resource path");
		return -1;
	}

	it = adapters_map.find(rp->GetID(br::ResourceIdentifier::GPU));
	if (it == adapters_map.end()) {
		logger->Error("ADL: Missing GPU id=%d",
			rp->GetID(br::ResourceIdentifier::GPU));
		return -2;
	}
	return it->second;
}

AMDPowerManager::ActivityPtr_t
AMDPowerManager::GetActivityInfo(int adapter_id) {
	std::map<int, ActivityPtr_t>::iterator a_it;
	a_it = activity_map.find(adapter_id);
	if (a_it == activity_map.end()) {
		logger->Error("ADL: GetActivity invalid adapter id");
		return nullptr;
	}
	return a_it->second;
}

PowerManager::PMResult
AMDPowerManager::GetActivity(int adapter_id) {
	int ADL_Err = ADL_ERR;

	CHECK_OD_VERSION(adapter_id);

	ActivityPtr_t activity(GetActivityInfo(adapter_id));
	if (activity == nullptr)
		return PMResult::ERR_RSRC_INVALID_PATH;

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("ADL: Cannot get GPU(s) activity");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADL_Err = ADL2_Overdrive5_CurrentActivity_Get(context, adapter_id, activity.get());
	ADL2_Main_Control_Destroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetLoad(br::ResourcePathPtr_t const & rp, uint32_t & perc) {
	PMResult pm_result;
	perc = 0;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	pm_result = GetActivity(adapter_id);
	if (pm_result != PMResult::OK) {
		return pm_result;
	}

	perc = activity_map[adapter_id]->iActivityPercent;
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetTemperature(br::ResourcePathPtr_t const & rp, uint32_t &celsius) {
	int ADL_Err = ADL_ERR;
	celsius = 0;
	ADLTemperature temp;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("ADL: Cannot get GPU(s) temperature");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADL_Err = ADL2_Overdrive5_Temperature_Get(context, adapter_id, 0, &temp);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Temperature not available [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}
	celsius = temp.iTemperature / 1000;
	logger->Debug("ADL: [A%d] Temperature : %d °C",
		adapter_id, celsius);

	ADL2_Main_Control_Destroy(context);
	return PMResult::OK;
}


/* Clock frequency */

PowerManager::PMResult
AMDPowerManager::GetClockFrequency(br::ResourcePathPtr_t const & rp, uint32_t &khz) {
	PMResult pm_result;
	br::ResourceIdentifier::Type_t r_type = rp->Type();
	khz = 0;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	pm_result = GetActivity(adapter_id);
	if (pm_result != PMResult::OK) {
		return pm_result;
	}

	if (r_type == br::ResourceIdentifier::PROC_ELEMENT)
		khz = activity_map[adapter_id]->iEngineClock * 10;
	else if (r_type == br::ResourceIdentifier::MEMORY)
		khz = activity_map[adapter_id]->iMemoryClock * 10;
	else {
		logger->Warn("ADL: Invalid resource path [%s]", rp->ToString().c_str());
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	return PMResult::OK;
}


PowerManager::PMResult
AMDPowerManager::GetClockFrequencyInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step) {
	khz_min = khz_max = khz_step = 0;
	br::ResourceIdentifier::Type_t r_type = rp->Type();
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	if (r_type == br::ResourceIdentifier::PROC_ELEMENT) {
		khz_min  = od_params_map[adapter_id].sEngineClock.iMin * 10;
		khz_max  = od_params_map[adapter_id].sEngineClock.iMax * 10;
		khz_step = od_params_map[adapter_id].sEngineClock.iStep * 10;
	}
	else if (r_type == br::ResourceIdentifier::MEMORY) {
		khz_min  = od_params_map[adapter_id].sMemoryClock.iMin * 10;
		khz_max  = od_params_map[adapter_id].sMemoryClock.iMax * 10;
		khz_step = od_params_map[adapter_id].sMemoryClock.iStep * 10;
	}
	else
		return PMResult::ERR_RSRC_INVALID_PATH;

	return PMResult::OK;
}


/* Voltage */

PowerManager::PMResult
AMDPowerManager::GetVoltage(br::ResourcePathPtr_t const & rp, uint32_t &mvolt) {
	mvolt = 0;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	ActivityPtr_t activity(GetActivityInfo(adapter_id));
	if (activity == nullptr) {
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	mvolt = activity_map[adapter_id]->iVddc;
	logger->Debug("ADL: [A%d] Voltage: %d mV", adapter_id, mvolt);

	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetVoltageInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &mvolt_min,
		uint32_t &mvolt_max,
		uint32_t &mvolt_step) {
	br::ResourceIdentifier::Type_t r_type = rp->Type();
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	if (r_type != br::ResourceIdentifier::PROC_ELEMENT) {
		logger->Error("ADL: Not a processing resource!");
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	mvolt_min  = od_params_map[adapter_id].sVddc.iMin;
	mvolt_max  = od_params_map[adapter_id].sVddc.iMax;
	mvolt_step = od_params_map[adapter_id].sVddc.iStep;

	return PMResult::OK;
}

/* Fan */

PowerManager::PMResult
AMDPowerManager::GetFanSpeed(
		br::ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value) {
	int ADL_Err = ADL_ERR;
	ADLFanSpeedValue fan;
	value = 0;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	// Fan speed type
	if (fs_type == FanSpeedType::PERCENT)
		fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;
	else if (fs_type == FanSpeedType::RPM)
		fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;

	ADL_Err = ADL2_Overdrive5_FanSpeed_Get(context, adapter_id, 0, &fan);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Fan speed adapter not available [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}
	value = fan.iFanSpeed;
	logger->Debug("ADL: [A%d] Fan speed: %d % ", adapter_id, value);

	ADL2_Main_Control_Destroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetFanSpeedInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step) {
	int ADL_Err = ADL_ERR;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed information");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADLFanSpeedInfo fan;
	ADL_Err  = ADL2_Overdrive5_FanSpeedInfo_Get(context, adapter_id, 0, &fan);
	rpm_min  = fan.iMinRPM;
	rpm_max  = fan.iMaxRPM;
	rpm_step = 0;

	ADL2_Main_Control_Destroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::SetFanSpeed(
		br::ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value)  {
	int ADL_Err = ADL_ERR;
	ADLFanSpeedValue fan;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	if (fs_type == FanSpeedType::PERCENT)
		fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;
	else if (fs_type == FanSpeedType::RPM)
		fan.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;

	fan.iFanSpeed = value;
	ADL_Err = ADL2_Overdrive5_FanSpeed_Set(context, adapter_id, 0, &fan);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Fan speed set failed [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	ADL2_Main_Control_Destroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::ResetFanSpeed(br::ResourcePathPtr_t const & rp) {
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	return _ResetFanSpeed(adapter_id);
}

PowerManager::PMResult
AMDPowerManager::_ResetFanSpeed(int adapter_id) {
	int ADL_Err = ADL_ERR;

	CHECK_OD_VERSION(adapter_id);

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADL_Err = ADL2_Overdrive5_FanSpeedToDefault_Set(context, adapter_id, 0);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Fan speed reset failed [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	ADL2_Main_Control_Destroy(context);
	return PMResult::OK;
}

/* Power */

PowerManager::PMResult
AMDPowerManager::GetPowerStatesInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &min,
		uint32_t &max,
		int &step) {
	int ADL_Err = ADL_ERR;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("ADL: Cannot get GPU(s) fan speed information");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADLPowerControlInfo pwr_info;
	ADL_Err = ADL2_Overdrive5_PowerControlInfo_Get(context, adapter_id, &pwr_info);
	min  = pwr_info.iMinValue;
	max  = pwr_info.iMaxValue;
	step = pwr_info.iStepValue;

	ADL2_Main_Control_Destroy(context);
	return PMResult::OK;
}

/* States */

PowerManager::PMResult
AMDPowerManager::GetPowerState(
		br::ResourcePathPtr_t const & rp,
		uint32_t & state) {
	int ADL_Err = ADL_ERR;
	int dflt;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("ADL: Cannot get GPU(s) power state");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	int amd_state;
	ADL_Err = ADL2_Overdrive5_PowerControl_Get(
		context, adapter_id, &amd_state, &dflt);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Power control not available [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}
	state = static_cast<uint32_t>(amd_state);

	ADL2_Main_Control_Destroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::SetPowerState(
		br::ResourcePathPtr_t const & rp,
		uint32_t state) {
	int ADL_Err = ADL_ERR;

	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);
	CHECK_OD_VERSION(adapter_id);

	ADL_Err = ADL2_Main_ControlX2_Create(
		ADL_Main_Memory_Alloc, 1, &context, ADL_THREADING_LOCKED);
	if (ADL_Err != ADL_OK) {
		logger->Error("ADL: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("ADL: Cannot set GPU(s) power state");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	ADL_Err = ADL2_Overdrive5_PowerControl_Set(context, adapter_id, state);
	if (ADL_Err != ADL_OK) {
		logger->Error(
			"ADL: [A%d] Power state set failed [%d]",
			adapter_id, ADL_Err);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	ADL2_Main_Control_Destroy(context);
	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetPerformanceState(
		br::ResourcePathPtr_t const & rp,
		uint32_t &state) {
	state = 0;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	ActivityPtr_t activity(GetActivityInfo(adapter_id));
	if (activity == nullptr)
		return PMResult::ERR_RSRC_INVALID_PATH;

	state = activity_map[adapter_id]->iCurrentPerformanceLevel,
	logger->Debug("ADL: [A%d] PerformanceState: %d ", adapter_id, state);

	return PMResult::OK;
}

PowerManager::PMResult
AMDPowerManager::GetPerformanceStatesCount(
		br::ResourcePathPtr_t const & rp,
		uint32_t &count) {
	count = 0;
	GET_PLATFORM_ADAPTER_ID(rp, adapter_id);

	if (!od_ready) {
		logger->Warn("ADL: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	CHECK_OD_VERSION(adapter_id);
	count = od_params_map[adapter_id].iNumberOfPerformanceLevels;

	return PMResult::OK;
}

} // namespace bbque

