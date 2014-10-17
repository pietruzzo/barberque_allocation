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

#ifndef BBQUE_POWER_MANAGER_AMD_H_
#define BBQUE_POWER_MANAGER_AMD_H_

#include <map>

#include "bbque/pm/power_manager.h"
#include "bbque/res/resource_path.h"
#include "bbque/pm/adl/adl_sdk.h"

#define AMD_VENDOR     "AMD"
#define ADL_NAME       "libatiadlxx.so"
#define ADL_OD_VERSION	5

extern "C" {

// Main
int ADL2_Main_ControlX2_Create (ADL_MAIN_MALLOC_CALLBACK, int, ADL_CONTEXT_HANDLE * , ADLThreadingModel);
int ADL2_Main_Control_Destroy (ADL_CONTEXT_HANDLE);

// Adapter
int ADL2_Adapter_Active_Get (ADL_CONTEXT_HANDLE, int, int* );
//int ADL_Adapter_AdapterInfo_Get ( LPAdapterInfo, int );
int ADL2_Adapter_Accessibility_Get (ADL_CONTEXT_HANDLE, int, int * );
int ADL2_Adapter_AdapterInfoX2_Get (ADL_CONTEXT_HANDLE, LPAdapterInfo * );
int ADL2_Adapter_Clock_Get (int, int* , int* );
int ADL2_Adapter_NumberOfAdapters_Get (ADL_CONTEXT_HANDLE, int* );

// OverDrive
int ADL2_Overdrive_Caps (ADL_CONTEXT_HANDLE, int, int * , int *, int *);

// OverDrive 5
int ADL2_Overdrive5_CurrentActivity_Get (ADL_CONTEXT_HANDLE, int, ADLPMActivity *);

int ADL2_Overdrive5_FanSpeed_Get (ADL_CONTEXT_HANDLE, int,  int, ADLFanSpeedValue *);
int ADL2_Overdrive5_FanSpeed_Set (ADL_CONTEXT_HANDLE, int,  int, ADLFanSpeedValue *);
int ADL2_Overdrive5_FanSpeedToDefault_Set (ADL_CONTEXT_HANDLE, int,  int);
int ADL2_Overdrive5_FanSpeedInfo_Get (ADL_CONTEXT_HANDLE, int, int, ADLFanSpeedInfo *);

int ADL2_Overdrive5_ODParameters_Get (ADL_CONTEXT_HANDLE, int, ADLODParameters *);

int ADL2_Overdrive5_PowerControl_Caps (ADL_CONTEXT_HANDLE, int, int *);
int ADL2_Overdrive5_PowerControl_Get (ADL_CONTEXT_HANDLE, int, int *, int *);
int ADL2_Overdrive5_PowerControl_Set (ADL_CONTEXT_HANDLE, int, int);
int ADL2_Overdrive5_PowerControlInfo_Get (ADL_CONTEXT_HANDLE, int, ADLPowerControlInfo *);

int ADL2_Overdrive5_Temperature_Get (ADL_CONTEXT_HANDLE, int, int, ADLTemperature *);

}

namespace br = bbque::res;

namespace bbque {


/**
 * @class AMDPowerManager
 *
 * Provide power management related API for AMD GPU devices, by extending @ref
 * PowerManager class.
 */
class AMDPowerManager: public PowerManager {

public:
	/**
	 * @see class PowerManager
	 */
	AMDPowerManager();

	~AMDPowerManager();

	/**
	 * @see class PowerManager
	 */
	PMResult GetLoad(br::ResourcePathPtr_t const & rp, uint32_t & perc);

	/**
	 * @see class PowerManager
	 */
	PMResult GetTemperature(
		br::ResourcePathPtr_t const & rp, uint32_t &celsius);

	/**
	 * @see class PowerManager
	 */
	PMResult GetClockFrequency(br::ResourcePathPtr_t const & rp, uint32_t &khz);

	/**
	 * @see class PowerManager
	 */
	PMResult GetClockFrequencyInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step);

	/**
	 * @see class PowerManager
	 */
	PMResult GetVoltage(br::ResourcePathPtr_t const & rp, uint32_t &mvolt);

	/**
	 * @see class PowerManager
	 */
	virtual PMResult GetVoltageInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &mvolt_min,
		uint32_t &mvolt_max,
		uint32_t &mvolt_step);

	/**
	 * @see class PowerManager
	 */
	PMResult GetFanSpeed(
		br::ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value);

	/**
	 * @see class PowerManager
	 */
	PMResult GetFanSpeedInfo(
		br::ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step);

	/**
	 * @see class PowerManager
	 */
	PMResult SetFanSpeed(
		br::ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value);

	/**
	 * @see class PowerManager
	 */
	PMResult ResetFanSpeed(br::ResourcePathPtr_t const & rp);

	/**
	 * @see class PowerManager
	 */
	PMResult GetPowerState(br::ResourcePathPtr_t const & rp, uint32_t & state);

	/**
	 * @see class PowerManager
	 */
	PMResult GetPowerStatesInfo(
		br::ResourcePathPtr_t const & rp, uint32_t & min, uint32_t & max, int & step);

	/**
	 * @see class PowerManager
	 */
	PMResult SetPowerState(br::ResourcePathPtr_t const & rp, uint32_t state);

	/**
	 * @see class PowerManager
	 */
	PMResult GetPerformanceState(
			br::ResourcePathPtr_t const & rp, uint32_t &state);

	/**
	 * @see class PowerManager
	 */
	PMResult GetPerformanceStatesCount(
			br::ResourcePathPtr_t const & rp, uint32_t &count);

private:

	bool initialized = false;

	typedef std::shared_ptr<ADLPMActivity> ActivityPtr_t;

	struct ODStatus_t {
		int supported;
		int enabled;
		int version;
	};

	/*** ADL context for thread-safety purpose */
	ADL_CONTEXT_HANDLE  context;

	/***  Pointer to the AMD Display Library  */
	void * adlib;

	/*** ADL parameters */
	ADLODParameters adl_od_params;

	bool od_ready;

	/*** Pointer to AdapterInfo data structure */
	LPAdapterInfo adapters_info = NULL;

	/*** Number of GPU adapters in the system */
	int adapters_count;

	/*** Mapping BBQ resource id -> ADL adapter id */
	std::map<br::ResID_t, int> adapters_map;

	/*** Activity resume for each adapter */
	std::map<int, ActivityPtr_t> activity_map;

	/*** Power control capabilities */
	std::map<int, int> power_caps_map;

	/*** AMD Overdrive status and information */
	std::map<int, ODStatus_t> od_status_map;

	/*** AMD Overdrive status parameters */
	std::map<int, ADLODParameters> od_params_map;

	/**
	 * @brief Load adapters information
	 */
	void LoadAdaptersInfo();

	/**
	 * @brief Get the platform adapter id (returned from ADL)
	 *
	 * @param rp Resource path of the GPU (including '.pe')
	 * @return The integer adapter id
	 */
	int GetAdapterId(br::ResourcePathPtr_t const & rp) const;

	/**
	 * @brief Get the platform adapter id (returned from ADL)
	 *
	 * @param rp Resource path of the GPU (including '.pe')
	 *
	 * @return PMResult::OK if success
	 */
	PMResult GetActivity(int adapter_id);

	/**
	 * @brief Get the activity data structure
	 *
	 * @param adapter_id The ADL adapter id
	 *
	 * @return A (shared) pointer to the activity data structure
	 */
	ActivityPtr_t GetActivityInfo(int adapter_id);

	/**
	 * @brief Reset fan speed to default value
	 *
	 * @param adapter_id The ADL adapter id
	 *
	 * @return PMResult::OK if success
	 */
	PMResult _ResetFanSpeed(int adapter_id);

};

}

#endif // BBQUE_POWER_MANAGER_AMD_H_

