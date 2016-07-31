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

#define  GET_DEVICE_ID(rp, device) \
 	nvmlDevice_t device; \
	GetDeviceId(rp, device); \
	if (device == NULL) { \
		logger->Error("NVML: The path does not resolve a resource"); \
		return PMResult::ERR_RSRC_INVALID_PATH; \
	}

/*#define CHECK_OD_VERSION(device)\
	if (od_status_map[device].version != NVML_OD_VERSION) {\
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
	logger->Warn("--- NVML: I am the LoadDevicesInfo");
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
            logger->Error("Failed to get name of device %i: %s", i, nvmlErrorString(result));
        }

		// pci.busId is very useful to know which device physically you're talking to
        // Using PCI identifier you can also match nvmlDevice handle to CUDA device.
        result = nvmlDeviceGetPciInfo(device, &device_info.pci);
        if (NVML_SUCCESS != result)
        { 
            logger->Error("Failed to get pci info for device %i: %s", i, nvmlErrorString(result));
        }

        logger->Debug("%d. %s [%s] %d", i, device_info.name, device_info.pci.busId, device);

        // Mapping information Devices per devices
		//info_map.insert(std::pair<nvmlDevice_t, DeviceInfo>(device, device_info));
        
        // Power control capabilities
        result = nvmlDeviceGetComputeMode(device, &device_info.compute_mode);
        if (NVML_ERROR_NOT_SUPPORTED == result)
            logger->Error("This is not CUDA capable device");
        else if (NVML_SUCCESS != result)
        { 
            logger->Error("Failed to get compute mode for device %i: %s", i, nvmlErrorString(result));
            continue;
        }
        else
        {
            // try to change compute mode
            logger->Debug("Changing device's compute mode from '%s' to '%s'", convertToComputeModeString(device_info.compute_mode), 
               convertToComputeModeString(NVML_COMPUTEMODE_PROHIBITED));

            result = nvmlDeviceSetComputeMode(device, NVML_COMPUTEMODE_PROHIBITED);
            if (NVML_ERROR_NO_PERMISSION == result)
                logger->Error("Need root privileges to do that: %s", nvmlErrorString(result));
            else if (NVML_ERROR_NOT_SUPPORTED == result)
                logger->Error("Compute mode prohibited not supported. You might be running on"
                       "windows in WDDM driver model or on non-CUDA capable GPU.");
            
            else if (NVML_SUCCESS != result)
            {
                logger->Error("Failed to set compute mode for device %i: %s", i, nvmlErrorString(result));
            } 
            else
            {
            	//All is gone correctly
            	logger->Debug("Device initialized correctly");
            	// Mapping information Devices per devices
				info_map.insert(std::pair<nvmlDevice_t, DeviceInfo>(device, device_info));
            }
        }
    }
	initialized = true;
	logger->Notice("NVIDIA: Devices [#=%d] information initialized", devices_map.size());

	std::map<br::ResID_t, nvmlDevice_t>::iterator it = devices_map.begin();
	for (; it!= devices_map.end(); ++it)
	{
		auto it2 = info_map.find(it->second);
		if (it2 != info_map.end())
		{
			logger->Debug("Restoring device's compute mode back to '%s'", convertToComputeModeString(it2->second.compute_mode));
	        result = nvmlDeviceSetComputeMode(it->second, it2->second.compute_mode);
	        if (NVML_SUCCESS != result)
	        { 
	        	logger->Error("Failed to restore compute mode for device %s: %s", it2->second.name, nvmlErrorString(result));
	        }
	        logger->Debug("Restored correctly");
		}
		else
			logger->Error("Error inside the matching between device_map and info_map");
		
	}
}

NVIDIAPowerManager::~NVIDIAPowerManager() 
{
	nvmlReturn_t result;
	logger->Warn("--- NVML: I am the NVIDIAPowerManager DESTROYER");

	std::map<br::ResID_t, nvmlDevice_t>::iterator it = devices_map.begin();
	for (; it!= devices_map.end(); ++it)
	{
		auto it2 = info_map.find(it->second);
		if (it2 != info_map.end())
		{
			logger->Debug("Restoring device's compute mode back to '%s'", convertToComputeModeString(it2->second.compute_mode));
	        result = nvmlDeviceSetComputeMode(it->second, it2->second.compute_mode);
	        if (NVML_SUCCESS != result)
	        { 
	        	logger->Error("Failed to restore compute mode for device %s: %s", it2->second.name, nvmlErrorString(result));
	        }
	        logger->Debug("Restored correctly");
		}
		else
			logger->Error("Error inside the matching between device_map and info_map");
		
	}

	result = nvmlShutdown();
    if (NVML_SUCCESS != result)
    {
        logger->Error("NVML: Failed to shutdown NVML: [Err:%s]", nvmlErrorString(result));
    }
    logger->Notice("NVML shutdownend correctly");

    devices_map.clear();
    info_map.clear();
}

int NVIDIAPowerManager::GetDeviceId(br::ResourcePathPtr_t const & rp, nvmlDevice_t &device) const 
{
	std::map<br::ResID_t, nvmlDevice_t>::const_iterator it;
	std::map<nvmlDevice_t, DeviceInfo>::const_iterator it2;
	//logger->Warn("--- NVML: I am the GetDeviceId");
	if (rp == nullptr) 
	{
		logger->Error("NVML: Null resource path");
		return -1;
	}

	it = devices_map.find(rp->GetID(br::ResourceIdentifier::GPU));
	it2 = info_map.find(it->second);
	if (it == devices_map.end()) 
	{
		logger->Error("NVML: Missing GPU id=%d",rp->GetID(br::ResourceIdentifier::GPU));
		return -2;
	}
	if (it2 == info_map.end()) 
	{
		logger->Error("NVML: Missing GPU id=%d information",rp->GetID(br::ResourceIdentifier::GPU));
		return -2;
	}

	//logger->Debug("NVML: GetDeviceId found GPU %d = Device %u %s",rp->GetID(br::ResourceIdentifier::GPU), it2->second.pci.device,it2->second.name);
	device = it->second;
	return 0;
}

PowerManager::PMResult
NVIDIAPowerManager::GetLoad(br::ResourcePathPtr_t const & rp, uint32_t & perc) 
{
	nvmlReturn_t result;
	nvmlUtilization_t utilization;
	
	GET_DEVICE_ID(rp, device);
	logger->Warn("--- NVML: I am the GetLoad");
	
	result = nvmlDeviceGetUtilizationRates(device, &utilization);
    if (NVML_SUCCESS != result)
    { 
    	logger->Error("NVML: Cannot get GPU (Device %d) load", device);
        logger->Error("NVML: Failed to to query the utilization rate for device %i: %s", device, nvmlErrorString(result));
    }

    logger->Debug("Gpu utilization rate %u ", utilization.gpu);
    logger->Debug("Gpu memory utilization rate %u", utilization.memory);
    perc = utilization.gpu;
	return PMResult::OK;
}

PowerManager::PMResult
NVIDIAPowerManager::GetTemperature(br::ResourcePathPtr_t const & rp, uint32_t &celsius) 
{
	nvmlReturn_t result;
	celsius = 0;
	unsigned int temp;
	
	GET_DEVICE_ID(rp, device);
	logger->Warn("--- NVML: I am the GetTemperature");

	if (!initialized) 
	{
		logger->Warn("NVML: Cannot get GPU(s) temperature");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	result = nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp);
    if (NVML_SUCCESS != result)
    { 
        logger->Error("NVML: Failed to to query the temperature: %s", nvmlErrorString(result));
        logger->Error("NVML: [A%d] Temperature not available [%d]",device, nvmlErrorString(result));
		return PMResult::ERR_API_INVALID_VALUE;
    }
    celsius = temp;
	logger->Debug("NVML: Temperature : %d C", temp);

	return PMResult::OK;
}


/* Clock frequency */
 
/*PowerManager::PMResult
NVIDIAPowerManager::GetAvailableFrequencies(br::ResourcePathPtr_t const & rp, std::vector<uint32_t> & freqs)
{
	nvmlReturn_t result;
	unsigned int clockMhz[10], count = 10, memoryClockMHz =2700;
	std::vector<uint32_t>::iterator it;

	
	GET_DEVICE_ID(rp, device);
	logger->Warn("--- NVML: I am the GetAvailableFrequencies");

    //unsigned int vet[nelement];
    logger->Warn("before \n");

    result = nvmlDeviceGetSupportedMemoryClocks(device , &count, clockMhz);

	//result = nvmlDeviceGetSupportedGraphicsClocks (device, memoryClockMHz , &count, clockMhz);
    if (NVML_SUCCESS != result)
    { 
        logger->Error("NVML: Failed to to query the supported graphic clock: %s",nvmlErrorString(result));
        return PMResult::ERR_API_INVALID_VALUE;
    }

    /*it = freqs.begin();

    for (int i = 0; it!= freqs.end(); ++i, it++)
    {
    	freqs.insert(it,clockMhz[i]);
    	logger->Debug("NVML: possible clock frequency: %d Mhz", clockMhz[i]);

    }
    
	return PMResult::OK;
}*/

PowerManager::PMResult
NVIDIAPowerManager::GetClockFrequency(br::ResourcePathPtr_t const & rp, uint32_t &khz) 
{
	nvmlReturn_t result;
	unsigned int var;
	khz = 0;

	
	GET_DEVICE_ID(rp, device);
	logger->Warn("--- NVML: I am the GetClockFrequency");

	if (!initialized) 
	{
		logger->Warn("NVML: Cannot get GPU(s) clock frequency");
		return PMResult::ERR_API_NOT_SUPPORTED;
	}

	result = nvmlDeviceGetClockInfo (device, NVML_CLOCK_GRAPHICS , &var);
    if (NVML_SUCCESS != result)
    { 
        logger->Error("NVML: Failed to to query the graphic clock: %s",nvmlErrorString(result));
        return PMResult::ERR_API_INVALID_VALUE;
    }

    khz = var;
	logger->Debug("NVML: [A%d] clock frequency: %d Mhz",device, var);

	return PMResult::OK;
}


PowerManager::PMResult
NVIDIAPowerManager::GetClockFrequencyInfo(br::ResourcePathPtr_t const & rp, uint32_t &khz_min, uint32_t &khz_max, uint32_t &khz_step) 
{
	nvmlReturn_t result;
	unsigned int var;
	khz_min = khz_max = khz_step = 0;
	br::ResourceIdentifier::Type_t r_type = rp->Type();

	GET_DEVICE_ID(rp, device);

	logger->Warn("NVML: I am the GetClockFrequencyInfo");
	if (r_type == br::ResourceIdentifier::PROC_ELEMENT) 
	{
		result = nvmlDeviceGetDefaultApplicationsClock(device, NVML_CLOCK_GRAPHICS, &var);
        if (NVML_SUCCESS != result)
        { 
            logger->Error("NVML: PROC_ELEMENT Failed to to query the default graphic clock: %s", nvmlErrorString(result));
            return PMResult::ERR_API_INVALID_VALUE;
        }
		khz_min  = var;
		logger->Debug("NVML: Min GPU freq %d Mhz", khz_min);
		result = nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_GRAPHICS, &var);
        if (NVML_SUCCESS != result)
        { 
        	logger->Error("NVML: Failed to to query the max graphic clock: %s", nvmlErrorString(result));
            return PMResult::ERR_API_INVALID_VALUE;
        }
		khz_max  = var;
		result = nvmlDeviceGetSupportedGraphicsClocks(device, NVML_CLOCK_GRAPHICS, &khz_step, &var);
		//khz_step = 1;
		logger->Debug("NVML: Max GPU freq %d Mhz", khz_max);
		logger->Debug("NVML: Step  freq %d Mhz", khz_step);
	}
	//else if (r_type == br::ResourceIdentifier::MEMORY) 
	{
		result = nvmlDeviceGetDefaultApplicationsClock(device, NVML_CLOCK_MEM, &var);
		if (NVML_SUCCESS != result)
        { 
           	logger->Error("NVML: MEMORY Failed to to query the default memory clock: %s", nvmlErrorString(result));
        	return PMResult::ERR_API_INVALID_VALUE;
        }
		khz_min  = var;
		logger->Debug("NVML: Min Mem freq %d Mhz", khz_min);
		result = nvmlDeviceGetMaxClockInfo (device, NVML_CLOCK_MEM , &var);
        if (NVML_SUCCESS != result)
        { 
            printf("NVML: Failed to to query the max memory clock: %s", nvmlErrorString(result));
            return PMResult::ERR_API_INVALID_VALUE;
        }
		khz_max  = var;
		nvmlDeviceGetSupportedGraphicsClocks(device, NVML_CLOCK_MEM, &khz_step, &var);
		//khz_step = 1;
		logger->Debug("NVML: Max Mem freq %d Mhz", khz_max);
		logger->Debug("NVML: Step freq %d Mhz", khz_step);
	}

	logger->Warn("NVML: I am going out GetClockFrequencyInfo");
	return PMResult::OK;
}


/* Fan */

PowerManager::PMResult
NVIDIAPowerManager::GetFanSpeed(br::ResourcePathPtr_t const & rp, FanSpeedType fs_type,	uint32_t &value) 
{
	nvmlReturn_t result;
	unsigned int var;
	value = 0;
	
	GET_DEVICE_ID(rp, device);
	logger->Warn("--- NVML: I am the GetFanSpeed");

	// Fan speed type
	if (fs_type == FanSpeedType::PERCENT)
	{
		result = nvmlDeviceGetFanSpeed (device, &var);
        if (NVML_SUCCESS != result)
        { 
            logger->Error("NVML: Failed to to query the fan speed: %s", nvmlErrorString(result));
            return PMResult::ERR_API_INVALID_VALUE;
        }
      	logger->Debug("NVML: Fan speed: %d%c ", var,'%');
      	value = (uint32_t) var;
	}
	else if (fs_type == FanSpeedType::RPM)
	{
      	value = 0;
	}

	return PMResult::OK;
}


/* Power */

PowerManager::PMResult 
NVIDIAPowerManager::GetPowerUsage(br::ResourcePathPtr_t const & rp, uint32_t &mwatt)
{
	nvmlReturn_t result;
	unsigned int var;
	
	GET_DEVICE_ID(rp, device);
	logger->Warn("--- NVML: I am the GetPowerUsage");


	result = nvmlDeviceGetPowerUsage (device, &var);
	if (NVML_SUCCESS != result)
    { 
        logger->Error("NVML: Failed to to query the power usage: %s", nvmlErrorString(result));
        return PMResult::ERR_API_INVALID_VALUE;
    }
    logger->Debug("NVML: the power usage of GPU is %d mW [+/-5%c]", var,'%');
	mwatt = var;

	return PMResult::OK;
}

PowerManager::PMResult 
NVIDIAPowerManager::GetPowerInfo(br::ResourcePathPtr_t const & rp, uint32_t &mwatt_min,uint32_t &mwatt_max)
{
	nvmlReturn_t result;
	unsigned int min, max;
	
	GET_DEVICE_ID(rp, device);
	logger->Warn("--- NVML: I am the GetPowerInfo");

	result = nvmlDeviceGetPowerManagementLimitConstraints(device, &min, &max);
	if (NVML_SUCCESS != result)
    { 
        logger->Error("NVML: Failed to to query the power information: %s", nvmlErrorString(result));
        return PMResult::ERR_API_INVALID_VALUE;
    }
    
	mwatt_min = min;
	mwatt_max = max;

	return PMResult::OK;
}

PowerManager::PMResult
NVIDIAPowerManager::GetPowerStatesInfo(br::ResourcePathPtr_t const & rp, uint32_t &min, uint32_t &max,	int &step) 
{
	logger->Warn("--- NVML: I am the GetPowerStatesInfo");
	
	min = 15;
	max  = 0;
	step = 1;

	return PMResult::OK;
}


/* States */

/*PowerManager::PMResult
NVIDIAPowerManager::GetPowerState(br::ResourcePathPtr_t const & rp, uint32_t & state) 
{
	nvmlPstates_t pState;
	nvmlReturn_t result;
	state = 0;
	GET_DEVICE_ID(rp, device);
	logger->Warn("--- NVML: I am the GetPowerState");

    result = nvmlDeviceGetPowerState (device, &pState);
    if (NVML_SUCCESS != result)
    { 
        logger->Error("NVML: Failed to to query the power state: %s",nvmlErrorString(result));
        return PMResult::ERR_API_INVALID_VALUE;
    }
    
    logger->Debug("Power state has an interval [0-15]+{32}:");
    logger->Debug("i)0 for Maximum Performance");
    logger->Debug("ii) 15 for Minimum Performance");
    logger->Debug("iii) 32 Unknown performance state");
	logger->Debug("NVML: PerformanceState: %u ", pState);
	state = (uint32_t) pState;

	return PMResult::OK;
}*/

/*PowerManager::PMResult
NVIDIAPowerManager::SetPowerState(br::ResourcePathPtr_t const & rp,uint32_t state) 
{
	int ADL_Err = ADL_ERR;

	GET_DEVICE_ID(rp, adapter_id);
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
}*/

PowerManager::PMResult
NVIDIAPowerManager::GetPerformanceState(br::ResourcePathPtr_t const & rp,uint32_t &state) 
{
	nvmlPstates_t pState;
	nvmlReturn_t result;
	state = 0;
	GET_DEVICE_ID(rp, device);
	logger->Warn("--- NVML: I am the GetPerformanceState");

    result = nvmlDeviceGetPowerState (device, &pState);
    if (NVML_SUCCESS != result)
    { 
        logger->Error("NVML: Failed to to query the power state: %s",nvmlErrorString(result));
        return PMResult::ERR_API_INVALID_VALUE;
    }
    
    logger->Debug("Power state has an interval [0-15]+{32}:");
    logger->Debug("i)0 for Maximum Performance");
    logger->Debug("ii) 15 for Minimum Performance");
    logger->Debug("iii) 32 Unknown performance state");
	logger->Debug("NVML: PerformanceState: %u ", pState);
	state = (uint32_t) pState;

	return PMResult::OK;
}

PowerManager::PMResult
NVIDIAPowerManager::GetPerformanceStatesCount(br::ResourcePathPtr_t const & rp,	uint32_t &count) 
{
	logger->Warn("--- NVML: I am the GetPerformanceStatesCount");
	count = 15;

	return PMResult::OK;
}

} // namespace bbque

