/*
 * Copyright (C) 2012  Politecnico di Milano
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

#ifndef BBQUE_RESOURCES_H_
#define BBQUE_RESOURCES_H_

#include <cstdint>
#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "bbque/config.h"
#include "bbque/app/application_status.h"
#include "bbque/pm/power_manager.h"
#include "bbque/res/identifier.h"
#include "bbque/utils/utility.h"
#include "bbque/utils/timer.h"
#include "bbque/utils/stats.h"

// Default number of samples for power profiling:
// {LOAD,TEMPERATURE,FREQUENCY,...}
// @see enum class InfoType
#define BBQUE_PM_DEFAULT_SAMPLES_WINSIZE 	{3,1,1,1}

using bbque::app::AppSPtr_t;
using bbque::app::AppUid_t;
using bbque::utils::ExtraDataContainer;
using bbque::utils::Timer;
using bbque::utils::pEma_t;

namespace bbque {

// Forward declaration
class ResourceAccounter;


namespace res {

// Forward declarations
class Resource;
struct ResourceState;

/** Resource state view token data type */
typedef size_t RViewToken_t;
/** Shared pointer to Resource descriptor */
typedef std::shared_ptr<Resource> ResourcePtr_t;
/** List of shared pointers to Resource descriptors */
typedef std::list<ResourcePtr_t> ResourcePtrList_t;
/** Iterator of ResourcePtr_t list */
typedef ResourcePtrList_t::iterator ResourcePtrListIterator_t;
/** Shared pointer to ResourceState object */
typedef std::shared_ptr<ResourceState> ResourceStatePtr_t;
/** Map of amounts of resource used by applications. Key: Application UID */
typedef std::map<AppUid_t, uint64_t> AppUseQtyMap_t;
/** Hash map collecting the state views of a resource */
typedef std::unordered_map<RViewToken_t, ResourceStatePtr_t> RSHashMap_t;


/**
 * @brief The status of a resource
 *
 * The class keep track of the status of the resource, from the usage standpoint.
 * How many resources are used/available? Which application is using the
 * resource? How much is using it ?
 * This are the basic information we need to track upon resources.
 *
 * @note: The total amount of resource is not an information of "state".
 * Indeed a state is a dynamic concept, while the total is a static
 * information.
 */
struct ResourceState {

	/**
	 * @brief Constructor
	 */
	ResourceState():
		used(0) {
	}

	/**
	 * @brief Destructor
	 */
	~ResourceState() {
		apps.clear();
	}

	/** The amount of resource used in the system   */
	uint64_t used;

	/**
	 * Amounts of resource used by each of the applications holding the
	 * resource
	 */
	AppUseQtyMap_t apps;

};


/**
 * @brief A generic resource descriptor
 *
 * Of course the "Resource" is the fundamental entity for BarbequeRTRM.
 * To access a resource is a matter of using a "path". A resource path is
 * built recursively, as a sequence of resources name, in a hierarchical
 * form (@see ResourceAccounter, @see ResourceTree).
 *
 * Basically, a resource has a identifying name, a total amount value,  and a
 * state. In our design, MORE than one state.
 * The idea is to have a default state, the "real" one, and a possible set of
 * temporary states to use as "buffers". Thus each state is a different VIEW
 * of resource. This feature is particularly useful for components like the
 * Scheduler/Optimizer (see below.)
 */
class Resource: public ResourceIdentifier, public ExtraDataContainer {

// This makes method SetTotal() accessible to RA
friend class bbque::ResourceAccounter;

public:

	enum ExitCode_t {
		/** Generic success code */
		RS_SUCCESS = 0,
		/** Generic failure code */
		RS_FAILED,
		/** The resource is not used by any application */
		RS_NO_APPS,
		/** Required a power information not enabled */
		RS_PWR_INFO_DISABLED
	};

	enum ValueType {
		INSTANT,
		MEAN
	};


	/**********************************************************************
	 * GENERAL INFORMATION                                                  *
	 **********************************************************************/

	/**
	 * @brief Constructor
	 * @param res_path Resource path
	 * @param tot The total amount of resource
	 */
	Resource(std::string const & res_path, uint64_t tot = 1);


	/**
	 * @brief Constructor
	 *
	 * @param type Resource type
	 * @param id Resource integer ID
	 * @param tot The total amount of resource
	 */
	Resource(ResourceIdentifier::Type_t type, BBQUE_RID_TYPE id, uint64_t tot = 1);

	/**
	 * Destructor
	 */
	~Resource() {
		state_views.clear();
#ifdef CONFIG_BBQUE_PM
		pw_profile.values.clear();
#endif
	}

	/**
	 * @brief Set the resource model name

	 * e.g. The model name of a CPU ("Intel i7-2640M")
	 */
	inline void SetModel(std::string const & _name) {
		model.assign(_name);
	}

	/**
	 * @brief Get the resource model name
	 *
	 * e.g. The model name of a CPU ("Intel i7-2640M")
	 *
	 * @return A char string object reference
	 */
	inline std::string const & Model() {
		return model;
	}

	/**
	 * @brief Set the resource path string
	 */
	inline void SetPath(std::string const & r_path) {
		path.assign(r_path);
	}

	/**
	 * @brief The registered resource path string
	 * @return A string containing the resource path
	 */
	inline std::string const & Path() {
		return path;
	}


	/**********************************************************************
	 * ACCOUNTING INFORMATION                                             *
	 **********************************************************************/

	/**
	 * @brief Resource total
	 * @return The total amount of resource
	 */
	inline uint64_t Total() {
		return total;
	}

	/**
	 * @brief Amount of resource used
	 *
	 * @param vtok The token referencing the resource view
	 *
	 * @return How much resource has been allocated
	 */
	uint64_t Used(RViewToken_t vtok = 0);

	/**
	 * @brief Resource availability
	 *
	 * @param papp Application interested in the query. We want to include
	 * in the count the amount of resource used by this application.
	 * This could be useful when we want to check the availability with the
	 * aim of allocate the resource to the given application. If the
	 * application is using yet a certain amount of resource this quantity
	 * should be considered as "available" for this application.
	 *
	 * If the Application is not specified the method returns the amount of
	 * resource free, i.e. not allocated to any Application/EXC.
	 *
	 * @param vtok The token referencing the resource view
	 *
	 * @return How much resource is still available including the amount of
	 * resource used by the given application
	 */
	uint64_t Available(AppSPtr_t papp = AppSPtr_t(), RViewToken_t vtok = 0);

	/**
	 * @brief Count of applications using the resource
	 *
	 * @param vtok The token referencing the resource view
	 * @return Number of applications
	 */
	inline uint16_t ApplicationsCount(RViewToken_t vtok = 0) {
		AppUseQtyMap_t apps_map;
		return ApplicationsCount(apps_map, vtok);
	}

	/**
	 * @brief Amount of resource used by the application
	 *
	 * @param papp Application (shared pointer) using the resource
	 * @param vtok The token referencing the resource view
	 *
	 * @return The 'quota' of resource used by the application
	 */
	uint64_t ApplicationUsage(AppSPtr_t const & papp, RViewToken_t vtok = 0);

	/**
	 * @brief Get the Uid of the idx-th App/EXC using the resource
	 *
	 * @param app_uid The Uid of the n-th App/EXC using the resource
	 * @param amount This is set to the amount of resource used by the App/EXC
	 * @param idx The n-th App/EXC to find
	 * @param vtok The token referencing the resource view
	 * @return RS_SUCCESS if the App/EXC has been found, RS_NO_APPS otherwise
	 */
	ExitCode_t UsedBy(
			AppUid_t & app_uid,
			uint64_t & amount,
			uint8_t idx = 0,
			RViewToken_t vtok = 0);


	/**
	 * @brief The number of state views of the resource
	 * @return The size of the map
	 */
	inline size_t ViewCount() {
		return state_views.size();
	}


	/**********************************************************************
	 * RUNTIME (PHYSICAL) AVAILABILITY                                    *
	 **********************************************************************/

	/**
	 * @brief Not reserved resources
	 *
	 * The amount of resources not being currently reserved, this value is
	 * equal to Total just when there are not reserved resources.
	 *
	 * @return The amount of resources not being currently reserved
	 */
	inline uint64_t Unreserved() {
		return (total - reserved);
	}

	/**
	 * @brief Make unavailable a given amount of resource
	 *
	 * @return
	 */
	ExitCode_t  Reserve(uint64_t amount);

	/**
	 * @brief Amount not available, not allocable
	 *
	 * @return The not allocable amount
	 */
	inline uint64_t Reserved() const {
		return reserved;
	}

	/**
	 * @brief Check if the resource is completely not available
	 *
	 * @return true if it is offline, false otherwise
	 */
	inline bool IsOffline() const {
		return offline;
	}

	/**
	 * @brief Make the resource completely not available
	 *
	 */
	void SetOffline();

	/**
	 * @brief Resume the availability of the resource
	 */
	void SetOnline();



#ifdef CONFIG_BBQUE_PM

	/**********************************************************************
	 * POWER INFORMATION                                                  *
	 **********************************************************************/

	/**
	 * @brief Enable the collection of power-thermal status information
	 *
	 * @param samples_window For each (required) power profile information
	 * compute a mean (exponential) value over a number of samples specified in
	 * the specific position (@see PowerManager::InfoType)
	 */
	void EnablePowerProfile(PowerManager::SamplesArray_t const & samples_window);

	/**
	 * @brief Enable the collection of power-thermal status information with
	 * the default setting specified in BBQUE_PM_DEFAULT_SAMPLES_WINSIZE
	 */
	void EnablePowerProfile();

	/**
	 * @brief The number of samples for the computation of the mean value of
	 * the power profile information required
	 *
	 * @param i_type The power profile information required
	 *
	 * @return The number of samples
	 */
	inline uint GetPowerInfoSamplesWindowSize(
			PowerManager::InfoType i_type) {
		return pw_profile.samples_window[int(i_type)];
	}

	/**
	 * @brief The number of power profile information required
	 */
	inline uint GetPowerInfoEnabledCount() {
		return pw_profile.enabled_count;
	}

	/**
	 * @brief Update the power profile information
	 *
	 * @param i_type The power profile information to update
	 * @param sample The sample value
	 */
	inline void UpdatePowerInfo(
			PowerManager::InfoType i_type, uint32_t sample) {
		pw_profile.values[int(i_type)]->update(sample);
	}

	/**
	 * @brief Power profile information
	 *
	 * @param i_type Information type (e.g., LOAD, TEMPERATURE, FREQUENCY,...)
	 * @param v_type Specify if the value required is the instantaneous or the
	 * mean (exponential) computed on a set of samples (@see ValueType)
	 *
	 * @return The value of power profile information required
	 */
	double GetPowerInfo(PowerManager::InfoType i_type, ValueType v_type = MEAN);

#endif // CONFIG_BBQUE_PM


	/**********************************************************************
	 * RELIABILITY INFORMATION                                            *
	 **********************************************************************/

	/**
	 * @brief Update the current performance degradation
	 *
	 * @param deg_perc Percentage of performance degradation
	 */
	 inline void UpdateDegradationPerc(uint8_t deg_perc) {
		rb_profile.degradation_perc->update(deg_perc);
	 }

	/**
	 * @brief The current performance degradation (last notification)
	 *
	 * @return Current percentage of performance degradation
	 */
	 inline uint8_t CurrentDegradationPerc() {
		return rb_profile.degradation_perc->last_value();
	 }

	/**
	 * @brief Performance degradation (exponential mean value)
	 *
	 * @return Mean percentage of performance degradation
	 */
	 inline float MeanDegradationPerc() {
		return rb_profile.degradation_perc->get();
	 }


private:

	/**
	 * @brief The metrics to track run-time availability of a resource
	 */
	typedef struct AvailabilityProfile {
		Timer online_tmr;         ///> Timer to keep track of online time;
		Timer offline_tmr;        ///> Timer to keep track of offline time;
		uint64_t lastOnlineTime;  ///> Last online timeframe [ms]
		uint64_t lastOfflineTime; ///> Last offline timeframe [ms]
	} AvailabilityProfile_t;


	/**
	 * @brief Information related to the power/thermal status of the resource
	 */
	typedef struct PowerProfile {
		/** Flags of the available run-time information */
		PowerManager::SamplesArray_t samples_window;
		/** Sampled values */
		std::vector<pEma_t> values;
		/** Count of power profiling info enabled */
		uint enabled_count;
	} PowerProfile_t;


	/**
	 * @brief Runtime information about the reliability of the resource
	 */
	typedef struct ReliabilityProfile {
		/** Percentage of degradation of the performance delivered
		 * (statistics) */
		pEma_t degradation_perc;
	} ReliabilityProfile_t;



	/** The total amount of resource  */
	uint64_t total;

	/** The amount of resource being reserved */
	uint64_t reserved;

	/** Former resource path string  */
	std::string path;

	/** Resource name, e.g. CPU architecture name */
	std::string model;

	/** True if this resource is currently offline */
	bool offline;


	/** The run-time availability profile of this resource */
	AvailabilityProfile_t av_profile;

#ifdef CONFIG_BBQUE_PM
	/** Power/thermal status (if the platform support is available) */
	PowerProfile_t pw_profile;

	PowerManager::SamplesArray_t default_samples_window =
		{BBQUE_PM_DEFAULT_SAMPLES_WINSIZE};
#endif

	/** The run-time reliability profile of this resource */
	ReliabilityProfile_t rb_profile;


	/**
	 * Hash map with all the views of the resource.
	 * A "view" is a resource state. We can think at the hash map as a map
	 * containing the "real" state of resource, plus other "temporary" states.
	 * Such temporary states allows the Scheduler/Optimizer, i.e., to make
	 * intermediate evaluations, before commit the ultimate scheduling.
	 *
	 * Each view is identified by a "token" which is hashed in order to
	 * retrieve the ResourceState descriptor.
	 *
	 * It's up to the Resource Accounter to maintain a consistent view of the
	 * system state. Thus ResourceAccounter will manage tokens and the state
	 * views life-cycle.
	 */
	RSHashMap_t state_views;

	/**
	 * @brief Availability information initialization
	 */
	void InitProfilingInfo();

	/**
	 * @brief Set the total amount of resource
	 *
	 * @note This method acts only upon the default state view only
	 * @param tot The amount to set
	 */
	inline void SetTotal(uint64_t tot) {
		total = tot;
	}

	/**
	 * @brief Acquire a given amount of resource
	 *
	 * @param papp The application requiring the resource
	 * @param amount How much resource is required
	 * @param vtok The token referencing the resource view
	 * @return The amount of resource acquired if success, 0 otherwise.
	 */
	 uint64_t Acquire(AppSPtr_t const & papp, uint64_t amount,
			 RViewToken_t vtok = 0);

	/**
	 * @brief Release the resource
	 *
	 * Release the specific amount of resource used by an application
	 *
	 * @param papp The application releasing the resource
	 * @param vtok The token referencing the resource view
	 * @return The amount of resource released
	 */
	uint64_t Release(AppSPtr_t const & papp, RViewToken_t vtok = 0);

	/**
	 * @brief Apps/EXCs using the resource
	 *
	 * @param apps_map Reference to the map of App/EXC to get
	 * @param vtok The resource state view token
	 *
	 * @return The number of Apps/EXCs using the resource, and a
	 * reference to the map
	 */
	uint16_t ApplicationsCount(AppUseQtyMap_t & apps_map,
			RViewToken_t vtok = 0);

	/**
	 * @brief Amount of resource used by the application
	 *
	 * @param papp Application (shared pointer) using the resource
	 * @param apps_map Reference to the map of App/EXC to get
	 *
	 * @return The 'quota' of resource used by the application
	 */
	uint64_t ApplicationUsage(AppSPtr_t const & papp, AppUseQtyMap_t & apps_map);

	/**
	 * @brief Get the view referenced by the token
	 *
	 * @param vtok The resource state view token
	 * @return The ResourceState fo the referenced view
	 */
	ResourceStatePtr_t GetStateView(RViewToken_t vtok);

	/**
	 * @brief Delete a state view
	 *
	 * If the token refers to the default view, the method returns doing
	 * nothing. This control is ahead of safety and consistency purposes.
	 * Indeed if the default view was removed, what state should be picked as
	 * the new default one?
	 * Thus, this constraint force the caller to set a new default view, before
	 * delete the current one.
	 *
	 * @param vtok The token of the view to delete
	 */
	void DeleteView(RViewToken_t vtok);
};


}   // namespace res

}   // namespace bbque

#endif // BBQUE_RESOURCES_H_
