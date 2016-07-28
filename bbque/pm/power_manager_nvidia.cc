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

#include "bbque/pm/power_manager_nvidia.h"

#define  GET_DEVICE_ID(device) \
	nvmlDevice_t device = GetDeviceId(rp); \
	if (device_id < 0) { \
		logger->Error("NVML: The path does not resolve a resource"); \
		return PMResult::ERR_RSRC_INVALID_PATH; \
	}

/*#define CHECK_OD_VERSION(device_id)\
	if (od_status_map[device_id].version != NVML_OD_VERSION) {\
		logger->Warn("NVML: Overdrive version %d not supported."\
			"Version %d expected", NVML_OD_VERSION); \
		return PMResult::ERR_API_VERSION;\
	}*/


const char * convertToComputeModeString(nvmlComputeMode_t mode)
{
    switch (mode)
    {
        case NVML_COMPUTEMODE_DEFAULT:
            return "Default";
        case NVML_COMPUTEMODE_EXCLUSIVE_THREAD:
            return "Exclusive_Thread";
        case NVML_COMPUTEMODE_PROHIBITED:
            return "Prohibited";
        case NVML_COMPUTEMODE_EXCLUSIVE_PROCESS:
            return "Exclusive Process";
        default:
            return "Unknown";
    }
}

namespace bbque {


/**   NVIDIA Display Library */

/*void* __stdcall NVML_Main_Memory_Alloc ( int iSize )
{
    void* lpBuffer = malloc ( iSize );
    return lpBuffer;
}

void __stdcall NVML_Main_Memory_Free ( void** lpBuffer )
{
    if ( NULL != *lpBuffer )
    {
        free ( *lpBuffer );
        *lpBuffer = NULL;
    }
}*/

NVIDIAPowerManager::NVIDIAPowerManager() {
	// Retrieve information about the GPU(s) of the system
	LoadDevicesInfo();
}

void NVIDIAPowerManager::LoadDevicesInfo() 
{
	nvmlReturn_t result;
	result = nvmlInit();
    if (NVML_SUCCESS != result)
    { 
    	logger->Error("NVML: Control initialization failed [Err:%s]", nvmlErrorString(result) );
        return;
    }
    logger->Info("NVML: Nvml inizializet correctly");

    // Devices enumeration
    result = nvmlDeviceGetCount(&device_count);
    if (NVML_SUCCESS != result)
    { 
        logger->Error("NVML: No device(s) available on the system [Err:%s]",nvmlErrorString(result));
        return;
    }
    logger->Info("NVML: Number of device(s) count = %d", device_count);

    for (uint i = 0; i < device_count; ++i) 
    {
    	DeviceInfo device_info;
        nvmlDevice_t device;

        // Query for device handle to perform operations on a device
        // You can also query device handle by other features like:
        // nvmlDeviceGetHandleBySerial
        // nvmlDeviceGetHandleByPciBusId
        result = nvmlDeviceGetHandleByIndex(i, &device);
        if (NVML_SUCCESS != result)
        { 
            logger->Debug("Skipping '%d' [Err:%d] ", i, nvmlErrorString(result));
            continue;
        }

		// Devices ID mapping and resouce path
		devices_map.insert(std::pair<br::ResID_t, nvmlDevice_t>(devices_map.size(), device));

		result = nvmlDeviceGetName(device, device_info.name, NVML_DEVICE_NAME_BUFFER_SIZE);
        if (NVML_SUCCESS != result)
        { 
            logger->Error("Failed to get name of device %i: %s\n", i, nvmlErrorString(result));
        }

		// pci.busId is very useful to know which device physically you're talking to
        // Using PCI identifier you can also match nvmlDevice handle to CUDA device.
        result = nvmlDeviceGetPciInfo(device, &device_info.pci);
        if (NVML_SUCCESS != result)
        { 
            logger->Error("Failed to get pci info for device %i: %s\n", i, nvmlErrorString(result));
        }

        logger->Debug("%d. %s [%s]\n", i, device_info.name, device_info.pci.busId);

        // Power control capabilities
        result = nvmlDeviceGetComputeMode(device, &device_info.compute_mode);
        if (NVML_ERROR_NOT_SUPPORTED == result)
            logger->Error("\t This is not CUDA capable device\n");
        else if (NVML_SUCCESS != result)
        { 
            logger->Error("Failed to get compute mode for device %i: %s\n", i, nvmlErrorString(result));
            continue;
        }
        else
        {
            // try to change compute mode
            logger->Debug("Changing device's compute mode from '%s' to '%s'\n", convertToComputeModeString(device_info.compute_mode), 
               convertToComputeModeString(NVML_COMPUTEMODE_PROHIBITED));

            result = nvmlDeviceSetComputeMode(device, NVML_COMPUTEMODE_PROHIBITED);
            if (NVML_ERROR_NO_PERMISSION == result)
                logger->Error("Need root privileges to do that: %s\n", nvmlErrorString(result));
            else if (NVML_ERROR_NOT_SUPPORTED == result)
                logger->Error("Compute mode prohibited not supported. You might be running on\n"
                       "windows in WDDM driver model or on non-CUDA capable GPU.\n");
            
            else if (NVML_SUCCESS != result)
            {
                logger->Error("Failed to set compute mode for device %i: %s\n", i, nvmlErrorString(result));
            } 
            else
            {
            	//All is gone correctly
            	logger->Debug("Device initialized correctly\n");
            }
        }
    }
	initialized = true;
	logger->Notice("NVIDIA: Devices [#=%d] information initialized", devices_map.size());
}

NVIDIAPowerManager::~NVIDIAPowerManager() 
{
	nvmlReturn_t result;
	std::map<br::ResID_t, nvmlDevice_t>::iterator it = devices_map.begin();
	for (; it!= devices_map.end(); ++it)
	{
		auto it2 = info_map.find(it->second);
		if (it2 != info_map.end())
		{
			printf("Restoring device's compute mode back to '%s'\n", convertToComputeModeString(it2->second.compute_mode));
	        result = nvmlDeviceSetComputeMode(it->second, it2->second.compute_mode);
	        if (NVML_SUCCESS != result)
	        { 
	        	logger->Error("Failed to restore compute mode for device %s: %s\n", it2->second.name, nvmlErrorString(result));
	        }
		}
		else
			logger->Error("Error inside the matching between device_map and info_map\n");
		
	}

	result = nvmlShutdown();
    if (NVML_SUCCESS != result)
    {
        logger->Error("NVML: Failed to shutdown NVML: [Err:%s]", nvmlErrorString(result));
    }

    devices_map.clear();
    info_map.clear();
}

int NVIDIAPowerManager::GetDeviceId(br::ResourcePathPtr_t const & rp, nvmlDevice_t &device) const 
{
	std::map<br::ResID_t, nvmlDevice_t>::const_iterator it;
	if (rp == nullptr) {
		logger->Error("NVML: Null resource path");
		return -1;
	}

	it = devices_map.find(rp->GetID(br::ResourceIdentifier::GPU));
	if (it == devices_map.end()) {
		logger->Error("NVML: Missing GPU id=%d",rp->GetID(br::ResourceIdentifier::GPU));
		return -2;
	}
	logger->Debug("NVML: GPU %d = Device %d",rp->GetID(br::ResourceIdentifier::GPU), it->second);
	device = it->second;
	return 0;
}

/*NVIDIAPowerManager::ActivityPtr_t
NVIDIAPowerManager::GetActivityInfo(int device_id) {
	std::map<int, ActivityPtr_t>::iterator a_it;
	a_it = activity_map.find(device_id);
	if (a_it == activity_map.end()) {
		logger->Error("NVML: GetActivity invalid device id");
		return nullptr;
	}
	return a_it->second;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::GetActivity(int device_id) {
	int result = result;

	CHECK_OD_VERSION(device_id);

	ActivityPtr_t activity(GetActivityInfo(device_id));
	if (activity == nullptr)
		return PMResult::ERR_RSRC_INVALID_PATH;

	result = NVML2_Main_ControlX2_Create(
		NVML_Main_Memory_Alloc, 1, &context, NVML_THREADING_LOCKED);
	if (result != NVML_SUCCESS) {
		logger->Error("NVML: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("NVML: Cannot get GPU(s) activity");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	result = NVML2_Overdrive5_CurrentActivity_Get(context, device_id, activity.get());
	NVML2_Main_Control_Destroy(context);
	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::GetLoad(br::ResourcePathPtr_t const & rp, uint32_t & perc) {
	PMResult pm_result;
	perc = 0;

	GET_PLATFORM_ADAPTER_ID(rp, device_id);
	pm_result = GetActivity(device_id);
	if (pm_result != PMResult::OK) {
		logger->Error("NVML: Cannot get GPU (Device %d) load", device_id);
		return pm_result;
	}

	perc = activity_map[device_id]->iActivityPercent;
	logger->Debug("NVML: [A%d] load = %3d", device_id, perc);
	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::GetTemperature(br::ResourcePathPtr_t const & rp, uint32_t &celsius) {
	nvmlReturn_t result;
	nvmlDevice_t device;
	celsius = 0;
	unsigned int *temp;
	logger->Warn("NVML: I am the GetTemperature");
	GET_PLATFORM_ADAPTER_ID(rp, device_id);
	//CHECK_OD_VERSION(device_id);

	if (!initialized) 
	{
		logger->Warn("NVML: Cannot get GPU(s) temperature");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	//result = nvmlDeviceGetTemperature(device_id, NVML_TEMPERATURE_GPU, temp);
	celsius = *temp;

    if (NVML_SUCCESS != result)
    { 
        logger->Error("NVML: Failed to to query the temperature: %s\n", nvmlErrorString(result));
        logger->Error("NVML: [A%d] Temperature not available [%d]",device_id, nvmlErrorString(result));
		return PMResult::ERR_API_INVALID_VALUE;
    }
	logger->Debug("NVML: [A%d] Temperature : %d °C",device_id, *temp);

	return PMResult::OK;
}*/


/* Clock frequency */

/*PowerManager::PMResult
NVIDIAPowerManager::GetClockFrequency(br::ResourcePathPtr_t const & rp, uint32_t &khz) {
	PMResult pm_result;
	br::ResourceIdentifier::Type_t r_type = rp->Type();
	khz = 0;

	GET_PLATFORM_ADAPTER_ID(rp, device_id);
	CHECK_OD_VERSION(device_id);

	pm_result = GetActivity(device_id);
	if (pm_result != PMResult::OK) {
		return pm_result;
	}

	if (r_type == br::ResourceIdentifier::PROC_ELEMENT)
		khz = activity_map[device_id]->iEngineClock * 10;
	else if (r_type == br::ResourceIdentifier::MEMORY)
		khz = activity_map[device_id]->iMemoryClock * 10;
	else {
		logger->Warn("NVML: Invalid resource path [%s]", rp->ToString().c_str());
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	return PMResult::OK;
}*/


/*PowerManager::PMResult
NVIDIAPowerManager::GetClockFrequencyInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step) {
	khz_min = khz_max = khz_step = 0;
	br::ResourceIdentifier::Type_t r_type = rp->Type();
	GET_PLATFORM_ADAPTER_ID(rp, device_id);

	if (!od_ready) {
		logger->Warn("NVML: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	if (r_type == br::ResourceIdentifier::PROC_ELEMENT) {
		khz_min  = od_params_map[device_id].sEngineClock.iMin * 10;
		khz_max  = od_params_map[device_id].sEngineClock.iMax * 10;
		khz_step = od_params_map[device_id].sEngineClock.iStep * 10;
	}
	else if (r_type == br::ResourceIdentifier::MEMORY) {
		khz_min  = od_params_map[device_id].sMemoryClock.iMin * 10;
		khz_max  = od_params_map[device_id].sMemoryClock.iMax * 10;
		khz_step = od_params_map[device_id].sMemoryClock.iStep * 10;
	}
	else
		return PMResult::ERR_RSRC_INVALID_PATH;

	return PMResult::OK;
}*/


/* Voltage */

/*PowerManager::PMResult
NVIDIAPowerManager::GetVoltage(br::ResourcePathPtr_t const & rp, uint32_t &mvolt) {
	mvolt = 0;
	GET_PLATFORM_ADAPTER_ID(rp, device_id);

	ActivityPtr_t activity(GetActivityInfo(device_id));
	if (activity == nullptr) {
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	mvolt = activity_map[device_id]->iVddc;
	logger->Debug("NVML: [A%d] Voltage: %d mV", device_id, mvolt);

	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::GetVoltageInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &mvolt_min,
		uint32_t &mvolt_max,
		uint32_t &mvolt_step) {
	br::ResourceIdentifier::Type_t r_type = rp->Type();
	GET_PLATFORM_ADAPTER_ID(rp, device_id);

	if (!od_ready) {
		logger->Warn("NVML: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	if (r_type != br::ResourceIdentifier::PROC_ELEMENT) {
		logger->Error("NVML: Not a processing resource!");
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	mvolt_min  = od_params_map[device_id].sVddc.iMin;
	mvolt_max  = od_params_map[device_id].sVddc.iMax;
	mvolt_step = od_params_map[device_id].sVddc.iStep;

	return PMResult::OK;
}*/

/* Fan */

/*PowerManager::PMResult
NVIDIAPowerManager::GetFanSpeed(
		br::ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value) {
	int result = result;
	nvmlUnitFanInfo_st fan;
	value = 0;
	GET_PLATFORM_ADAPTER_ID(rp, device_id);
	CHECK_OD_VERSION(device_id);

	result = NVML2_Main_ControlX2_Create(
		NVML_Main_Memory_Alloc, 1, &context, NVML_THREADING_LOCKED);
	if (result != NVML_SUCCESS) {
		logger->Error("NVML: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("NVML: Cannot get GPU(s) fan speed");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	// Fan speed type
	if (fs_type == FanSpeedType::PERCENT)
		fan.iSpeedType = NVML_DL_FANCTRL_SPEED_TYPE_PERCENT;
	else if (fs_type == FanSpeedType::RPM)
		fan.iSpeedType = NVML_DL_FANCTRL_SPEED_TYPE_RPM;

	result = NVML2_Overdrive5_FanSpeed_Get(context, device_id, 0, &fan);
	if (result != NVML_SUCCESS) {
		logger->Error(
			"NVML: [A%d] Fan speed device not available [%d]",
			device_id, result);
		return PMResult::ERR_API_INVALID_VALUE;
	}
	value = fan.iFanSpeed;
	logger->Debug("NVML: [A%d] Fan speed: %d % ", device_id, value);

	NVML2_Main_Control_Destroy(context);
	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::GetFanSpeedInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step) {
	int result = result;
	GET_PLATFORM_ADAPTER_ID(rp, device_id);

	if (!od_ready) {
		logger->Warn("NVML: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	result = NVML2_Main_ControlX2_Create(
		NVML_Main_Memory_Alloc, 1, &context, NVML_THREADING_LOCKED);
	if (result != NVML_SUCCESS) {
		logger->Error("NVML: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("NVML: Cannot get GPU(s) fan speed information");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	NVMLFanSpeedInfo fan;
	result  = NVML2_Overdrive5_FanSpeedInfo_Get(context, device_id, 0, &fan);
	rpm_min  = fan.iMinRPM;
	rpm_max  = fan.iMaxRPM;
	rpm_step = 0;

	NVML2_Main_Control_Destroy(context);
	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::SetFanSpeed(
		br::ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value)  {
	int result = result;
	NVMLFanSpeedValue fan;

	GET_PLATFORM_ADAPTER_ID(rp, device_id);
	CHECK_OD_VERSION(device_id);

	result = NVML2_Main_ControlX2_Create(
		NVML_Main_Memory_Alloc, 1, &context, NVML_THREADING_LOCKED);
	if (result != NVML_SUCCESS) {
		logger->Error("NVML: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("NVML: Cannot get GPU(s) fan speed");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	if (fs_type == FanSpeedType::PERCENT)
		fan.iSpeedType = NVML_DL_FANCTRL_SPEED_TYPE_PERCENT;
	else if (fs_type == FanSpeedType::RPM)
		fan.iSpeedType = NVML_DL_FANCTRL_SPEED_TYPE_RPM;

	fan.iFanSpeed = value;
	result = NVML2_Overdrive5_FanSpeed_Set(context, device_id, 0, &fan);
	if (result != NVML_SUCCESS) {
		logger->Error(
			"NVML: [A%d] Fan speed set failed [%d]",
			device_id, result);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	NVML2_Main_Control_Destroy(context);
	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::ResetFanSpeed(br::ResourcePathPtr_t const & rp) {
	GET_PLATFORM_ADAPTER_ID(rp, device_id);

	return _ResetFanSpeed(device_id);
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::_ResetFanSpeed(int device_id) {
	int result = result;

	CHECK_OD_VERSION(device_id);

	result = NVML2_Main_ControlX2_Create(
		NVML_Main_Memory_Alloc, 1, &context, NVML_THREADING_LOCKED);
	if (result != NVML_SUCCESS) {
		logger->Error("NVML: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("NVML: Cannot get GPU(s) fan speed");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	result = NVML2_Overdrive5_FanSpeedToDefault_Set(context, device_id, 0);
	if (result != NVML_SUCCESS) {
		logger->Error(
			"NVML: [A%d] Fan speed reset failed [%d]",
			device_id, result);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	NVML2_Main_Control_Destroy(context);
	return PMResult::OK;
}*/

/* Power */

/*PowerManager::PMResult
NVIDIAPowerManager::GetPowerStatesInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &min,
		uint32_t &max,
		int &step) {
	int result = result;
	GET_PLATFORM_ADAPTER_ID(rp, device_id);

	if (!od_ready) {
		logger->Warn("NVML: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	result = NVML2_Main_ControlX2_Create(
		NVML_Main_Memory_Alloc, 1, &context, NVML_THREADING_LOCKED);
	if (result != NVML_SUCCESS) {
		logger->Error("NVML: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("NVML: Cannot get GPU(s) fan speed information");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	NVMLPowerControlInfo pwr_info;
	result = NVML2_Overdrive5_PowerControlInfo_Get(context, device_id, &pwr_info);
	min  = pwr_info.iMinValue;
	max  = pwr_info.iMaxValue;
	step = pwr_info.iStepValue;

	NVML2_Main_Control_Destroy(context);
	return PMResult::OK;
}*/

/* States */

/*PowerManager::PMResult
NVIDIAPowerManager::GetPowerState(
		br::ResourcePathPtr_t const & rp,
		uint32_t & state) {
	int result = result;
	int dflt;

	GET_PLATFORM_ADAPTER_ID(rp, device_id);
	CHECK_OD_VERSION(device_id);

	result = NVML2_Main_ControlX2_Create(
		NVML_Main_Memory_Alloc, 1, &context, NVML_THREADING_LOCKED);
	if (result != NVML_SUCCESS) {
		logger->Error("NVML: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("NVML: Cannot get GPU(s) power state");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	int nvidia_state;
	result = NVML2_Overdrive5_PowerControl_Get(
		context, device_id, &nvidia_state, &dflt);
	if (result != NVML_SUCCESS) {
		logger->Error(
			"NVML: [A%d] Power control not available [%d]",
			device_id, result);
		return PMResult::ERR_API_INVALID_VALUE;
	}
	state = static_cast<uint32_t>(nvidia_state);

	NVML2_Main_Control_Destroy(context);
	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::SetPowerState(
		br::ResourcePathPtr_t const & rp,
		uint32_t state) {
	int result = result;

	GET_PLATFORM_ADAPTER_ID(rp, device_id);
	CHECK_OD_VERSION(device_id);

	result = NVML2_Main_ControlX2_Create(
		NVML_Main_Memory_Alloc, 1, &context, NVML_THREADING_LOCKED);
	if (result != NVML_SUCCESS) {
		logger->Error("NVML: Control initialization failed");
		return PMResult::ERR_API_INVALID_VALUE;
	}

	if (!initialized) {
		logger->Warn("NVML: Cannot set GPU(s) power state");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	result = NVML2_Overdrive5_PowerControl_Set(context, device_id, state);
	if (result != NVML_SUCCESS) {
		logger->Error(
			"NVML: [A%d] Power state set failed [%d]",
			device_id, result);
		return PMResult::ERR_API_INVALID_VALUE;
	}

	NVML2_Main_Control_Destroy(context);
	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::GetPerformanceState(
		br::ResourcePathPtr_t const & rp,
		uint32_t &state) {
	state = 0;
	GET_PLATFORM_ADAPTER_ID(rp, device_id);

	ActivityPtr_t activity(GetActivityInfo(device_id));
	if (activity == nullptr)
		return PMResult::ERR_RSRC_INVALID_PATH;

	state = activity_map[device_id]->iCurrentPerformanceLevel,
	logger->Debug("NVML: [A%d] PerformanceState: %d ", device_id, state);

	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::GetPerformanceStatesCount(
		br::ResourcePathPtr_t const & rp,
		uint32_t &count) {
	count = 0;
	GET_PLATFORM_ADAPTER_ID(rp, device_id);

	if (!od_ready) {
		logger->Warn("NVML: Overdrive parameters missing");
		return PMResult::ERR_NOT_INITIALIZED;
	}

	CHECK_OD_VERSION(device_id);
	count = od_params_map[device_id].iNumberOfPerformanceLevels;

	return PMResult::OK;
}*/

} // namespace bbque

