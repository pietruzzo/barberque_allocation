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

#include "bbque/pm/power_manager.h"
#include "bbque/pm/adl/adl_sdk.h"

#define AMD_VENDOR     "AMD"
#define ADL_NAME       "libatiadlxx.so"
#define ADL_OD_VERSION	5


/** ADL **/
#define SYM_ADL_MAIN_CTRL_CREATE   "ADL2_Main_ControlX2_Create"
typedef int ( *ADL_MAIN_CTRL_CREATE )
	( ADL_MAIN_MALLOC_CALLBACK, int, ADL_CONTEXT_HANDLE * , ADLThreadingModel);

#define SYM_ADL_MAIN_CTRL_DESTROY  "ADL2_Main_Control_Destroy"
typedef int ( *ADL_MAIN_CTRL_DESTROY )(ADL_CONTEXT_HANDLE);

#define SYM_ADL_ADAPTER_NUMBER_GET "ADL2_Adapter_NumberOfAdapters_Get"
typedef int ( *ADL_ADAPTER_NUMBER_GET ) (ADL_CONTEXT_HANDLE, int* );

#define SYM_ADL_ADAPTER_ACTIVE_GET "ADL2_Adapter_Active_Get"
typedef int ( *ADL_ADAPTER_ACTIVE_GET ) (ADL_CONTEXT_HANDLE, int, int* );

//#define SYM_ADL_ADAPTER_INFO_GET   "ADL_Adapter_AdapterInfo_Get"
//typedef int ( *ADL_ADAPTER_INFO_GET ) ( LPAdapterInfo, int );

#define SYM_ADL_ADAPTER_INFO_GET   "ADL2_Adapter_AdapterInfoX2_Get"
typedef int ( *ADL_ADAPTER_INFO_GET ) (ADL_CONTEXT_HANDLE, LPAdapterInfo * );

#define SYM_ADL_ADAPTER_ACCESS_GET "ADL2_Adapter_Accessibility_Get"
typedef int ( *ADL_ADAPTER_ACCESS_GET ) (ADL_CONTEXT_HANDLE, int, int * );

#define SYM_ADL_OD_CURRACTIVITY_GET "ADL2_Overdrive5_CurrentActivity_Get"
typedef int ( *ADL_OD_CURRACTIVITY_GET) (ADL_CONTEXT_HANDLE, int, ADLPMActivity *);

#define SYM_ADL_OD_TEMPERATURE_GET  "ADL2_Overdrive5_Temperature_Get"
typedef int ( *ADL_OD_TEMPERATURE_GET) (ADL_CONTEXT_HANDLE, int, int, ADLTemperature *);

#define SYM_ADL_OD_FANSPEED_GET  "ADL2_Overdrive5_FanSpeed_Get"
typedef int ( *ADL_OD_FANSPEED_GET ) (ADL_CONTEXT_HANDLE, int,  int, ADLFanSpeedValue *);

#define SYM_ADL_OD_FANSPEED_SET  "ADL2_Overdrive5_FanSpeed_Set"
typedef int ( *ADL_OD_FANSPEED_SET ) (ADL_CONTEXT_HANDLE, int,  int, ADLFanSpeedValue *);

#define SYM_ADL_OD_FANSPEED2DF_SET  "ADL2_Overdrive5_FanSpeedToDefault_Set"
typedef int ( *ADL_OD_FANSPEED2DF_SET ) (ADL_CONTEXT_HANDLE, int,  int);

#define SYM_ADL_OD_FANSPEEDINFO_GET  "ADL2_Overdrive5_FanSpeedInfo_Get"
typedef int ( *ADL_OD_FANSPEEDINFO_GET ) (ADL_CONTEXT_HANDLE, int, int, ADLFanSpeedInfo *);

#define SYM_ADL_OD_POWERCTRL_CAPS  "ADL2_Overdrive5_PowerControl_Caps"
typedef int ( *ADL_OD_POWERCTRL_CAPS) (ADL_CONTEXT_HANDLE, int, int *);

#define SYM_ADL_OD_POWERCTRL_GET  "ADL2_Overdrive5_PowerControl_Get"
typedef int ( *ADL_OD_POWERCTRL_GET) (ADL_CONTEXT_HANDLE, int, int *, int *);

#define SYM_ADL_OD_POWERCTRL_SET  "ADL2_Overdrive5_PowerControl_Set"
typedef int ( *ADL_OD_POWERCTRL_SET) (ADL_CONTEXT_HANDLE, int, int);

#define SYM_ADL_OD_POWERCTRLINFO_GET  "ADL2_Overdrive5_PowerControlInfo_Get"
typedef int ( *ADL_OD_POWERCTRLINFO_GET) (ADL_CONTEXT_HANDLE, int, ADLPowerControlInfo *);

#define SYM_ADL_OD_CAPS  "ADL2_Overdrive_Caps"
typedef int ( *ADL_OD_CAPS) (ADL_CONTEXT_HANDLE, int, int * , int *, int *);

#define SYM_ADL_OD_PARAMS_GET  "ADL2_Overdrive5_ODParameters_Get"
typedef int ( *ADL_OD_PARAMS_GET) (ADL_CONTEXT_HANDLE, int, ADLODParameters *);


typedef int ( *ADL_ADAPTER_CLOCK_GET) ( int, int* , int* );
/** */

using namespace bbque::res;


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
	AMDPowerManager(ResourcePathPtr_t rp, std::string const & vendor);

	~AMDPowerManager();

	/**
	 * @see class PowerManager
	 */
	PMResult GetLoad(ResourcePathPtr_t const & rp, uint32_t & perc);

	/**
	 * @see class PowerManager
	 */
	PMResult GetTemperature(
		ResourcePathPtr_t const & rp, uint32_t &celsius);

	/**
	 * @see class PowerManager
	 */
	PMResult GetClockFrequency(ResourcePathPtr_t const & rp, uint32_t &khz);

	/**
	 * @see class PowerManager
	 */
	PMResult GetClockFrequencyInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &khz_min,
		uint32_t &khz_max,
		uint32_t &khz_step);

	/**
	 * @see class PowerManager
	 */
	PMResult GetVoltage(ResourcePathPtr_t const & rp, uint32_t &mvolt);

	/**
	 * @see class PowerManager
	 */
	virtual PMResult GetVoltageInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &mvolt_min,
		uint32_t &mvolt_max,
		uint32_t &mvolt_step);

	/**
	 * @see class PowerManager
	 */
	PMResult GetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t &value);

	/**
	 * @see class PowerManager
	 */
	PMResult GetFanSpeedInfo(
		ResourcePathPtr_t const & rp,
		uint32_t &rpm_min,
		uint32_t &rpm_max,
		uint32_t &rpm_step);

	/**
	 * @see class PowerManager
	 */
	PMResult SetFanSpeed(
		ResourcePathPtr_t const & rp,
		FanSpeedType fs_type,
		uint32_t value);

	/**
	 * @see class PowerManager
	 */
	PMResult ResetFanSpeed(ResourcePathPtr_t const & rp);

	/**
	 * @see class PowerManager
	 */
	PMResult GetPowerState(ResourcePathPtr_t const & rp, int & state);

	/**
	 * @see class PowerManager
	 */
	PMResult GetPowerStatesInfo(
		ResourcePathPtr_t const & rp, int & min, int & max, int & step);

	/**
	 * @see class PowerManager
	 */
	PMResult SetPowerState(ResourcePathPtr_t const & rp, int state);

	/**
	 * @see class PowerManager
	 */
	PMResult GetPerformanceState(ResourcePathPtr_t const & rp, int &state);

	/**
	 * @see class PowerManager
	 */
	PMResult GetPerformanceStatesCount(ResourcePathPtr_t const & rp, int &count);

private:

	typedef std::shared_ptr<ADLPMActivity> ActivityPtr_t;

	struct ODStatus_t {
		int supported;
		int enabled;
		int version;
	};

	/** Pointers to ADL functions */

	ADL_MAIN_CTRL_CREATE   MainCtrlCreate;
	ADL_MAIN_CTRL_DESTROY  MainCtrlDestroy;
	ADL_ADAPTER_CLOCK_GET    AdapterObservedClockInfoGet;
	ADL_ADAPTER_NUMBER_GET   AdapterNumberGet;
	ADL_ADAPTER_ACTIVE_GET   AdapterActiveGet;
	ADL_ADAPTER_INFO_GET     AdapterInfoGet;
	ADL_ADAPTER_ACCESS_GET   AdapterAccessGet;
	ADL_OD_CAPS              ODCaps;
	ADL_OD_PARAMS_GET        ODParamsGet;
	ADL_OD_TEMPERATURE_GET   ODTemperatureGet;
	ADL_OD_FANSPEED_GET      ODFanSpeedGet;
	ADL_OD_FANSPEEDINFO_GET  ODFanSpeedInfoGet;
	ADL_OD_FANSPEED_SET      ODFanSpeedSet;
	ADL_OD_FANSPEED2DF_SET   ODFanSpeed2DfSet;
	ADL_OD_CURRACTIVITY_GET  ODCurrentActivityGet;
	ADL_OD_POWERCTRL_GET      ODPowerCtrlGet;
	ADL_OD_POWERCTRL_SET      ODPowerCtrlSet;
	ADL_OD_POWERCTRL_CAPS     ODPowerCtrlCaps;
	ADL_OD_POWERCTRLINFO_GET  ODPowerCtrlInfoGet;

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
	std::map<ResID_t, int> adapters_map;

	/*** Activity resume for each adapter */
	std::map<int, ActivityPtr_t> activity_map;

	/*** Power control capabilities */
	std::map<int, int> power_caps_map;

	/*** AMD Overdrive status and information */
	std::map<int, ODStatus_t> od_status_map;

	/*** AMD Overdrive status parameters */
	std::map<int, ADLODParameters> od_params_map;

	/**
	 * @brief Initialized ADL function pointers
	 *
	 * @return 0 if success, 1 otherwise
	 */
	int InitLibrary();

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
	int GetAdapterId(ResourcePathPtr_t const & rp) const;

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

