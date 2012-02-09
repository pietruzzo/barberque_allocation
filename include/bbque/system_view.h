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

#ifndef BBQUE_SYSTEM_VIEW_H_
#define BBQUE_SYSTEM_VIEW_H_

#include "bbque/application_manager.h"
#include "bbque/res/resource_accounter.h"

using bbque::ApplicationManager;
using bbque::ApplicationManagerStatusIF;
using bbque::app::ApplicationStatusIF;
using bbque::res::ResourceAccounter;
using bbque::res::ResourceAccounterStatusIF;
using bbque::res::ResourcePtr_t;
using bbque::res::ResourcePtrList_t;

namespace bbque {

/**
 * @brief A unified view on system status
 *
 * This class provides all the methods necessary to get an aggregated view of
 * the system status, where under "system" we put the set of applications and
 * resources managed by the RTRM.  When instanced, the class get references to
 * the ApplicationManager and ResourceAccounter instances and it provides a
 * simplified set of methods for making queries upon applications and
 * resources status.
 */
class SystemView {

public:

	/**
	 * @brief Get the SystemVIew instance
	 */
	static SystemView & GetInstance() {
		static SystemView instance;
		return instance;
	}

	/// ...........................: APPLICATIONS :...........................

	/**
	 * @brief Return the first app at the specified priority
	 */
	inline AppPtr_t GetFirstWithPrio(AppPrio_t prio, AppsUidMapIt & ait) {
		return am.GetFirst(prio, ait);
	}

	/**
	 * @brief Return the next app at the specified priority
	 */
	inline AppPtr_t GetNextWithPrio(AppPrio_t prio, AppsUidMapIt & ait) {
		return am.GetNext(prio, ait);
	}


	/**
	 * @brief Return the map containing all the ready applications
	 */
	inline AppPtr_t GetFirstReady(AppsUidMapIt & ait) {
		return am.GetFirst(ApplicationStatusIF::READY, ait);
	}

	inline AppPtr_t GetNextReady(AppsUidMapIt & ait) {
		return am.GetNext(ApplicationStatusIF::READY, ait);
	}

	/**
	 * @brief Map of running applications (descriptors)
	 */
	inline AppPtr_t GetFirstRunning(AppsUidMapIt & ait) {
		return am.GetFirst(ApplicationStatusIF::RUNNING, ait);
	}

	inline AppPtr_t GetNextRunning(AppsUidMapIt & ait) {
		return am.GetNext(ApplicationStatusIF::RUNNING, ait);
	}

	/**
	 * @brief Map of blocked applications (descriptors)
	 */

	inline AppPtr_t GetFirstBlocked(AppsUidMapIt & ait) {
		return am.GetFirst(ApplicationStatusIF::BLOCKED, ait);
	}

	inline AppPtr_t GetNextBlocked(AppsUidMapIt & ait) {
		return am.GetNext(ApplicationStatusIF::BLOCKED, ait);
	}

	/**
	 * @see ApplicationManagerStatusIF
	 */
	inline bool HasApplications(AppPrio_t prio) {
		return am.HasApplications(prio);
	}

	/**
	 * @see ApplicationManagerStatusIF
	 */
	inline bool HasApplications(ApplicationStatusIF::State_t state) {
		return am.HasApplications(state);
	}

	/**
	 * @see ApplicationManagerStatusIF
	 */
	inline bool HasApplications(ApplicationStatusIF::SyncState_t sync_state) {
		return am.HasApplications(sync_state);
	}

	/**
	 * @brief Maximum integer value for the minimum application priority
	 */
	inline uint16_t ApplicationLowestPriority() const {
		return am.LowestPriority();
	}


	/// .............................: RESOURCES :............................

	/**
	 * @see ResourceAccounterStatusIF::Available()
	 */
	inline uint64_t ResourceAvailable(std::string const & path,
			RViewToken_t vtok = 0, AppPtr_t papp = AppPtr_t()) const {
		return ra.Available(path, vtok, papp);
	}

	/**
	 * @see ResourceAccounterStatusIF::Available()
	 */
	inline uint64_t ResourceAvailable(ResourcePtrList_t & rsrc_list,
			RViewToken_t vtok = 0, AppPtr_t papp = AppPtr_t()) const {
		return ra.Available(rsrc_list, vtok, papp);
	}

	/**
	 * @see ResourceAccounterStatusIF::Total()
	 */
	inline uint64_t ResourceTotal(std::string const & path) const {
		return ra.Total(path);
	}

	/**
	 * @see ResourceAccounterStatusIF::Total()
	 */
	inline uint64_t ResourceTotal(ResourcePtrList_t & rsrc_list) const {
		return ra.Total(rsrc_list);
	}

	/**
	 * @see ResourceAccounterStatusIF::Used()
	 */
	inline uint64_t ResourceUsed(std::string const & path) const {
		return ra.Used(path);
	}

	/**
	 * @see ResourceAccounterStatusIF::Used()
	 */
	inline uint64_t ResourceUsed(ResourcePtrList_t & rsrc_list) const {
		return ra.Used(rsrc_list);
	}

	/**
	 * @see ResourceAccounterStatusIF::GetResource()
	 */
	inline ResourcePtr_t GetResource(std::string const & path) const {
		return ra.GetResource(path);
	}

	/**
	 * @see ResourceAccounterStatusIF::GetResources()
	 */
	inline ResourcePtrList_t GetResources(std::string const & temp_path)
		const {
		return ra.GetResources(temp_path);
	}

	/**
	 * @see ResourceAccounterStatusIF::ExistResources()
	 */
	inline bool ExistResource(std::string const & path) const {
		return ra.ExistResource(path);
	}

	/**
	 * @brief The number of system resources
	 * @return This returns the total number of system resources, i.e. the
	 * number of resource allocables to the applications.
	 */
	inline uint16_t GetTotalNumOfResources() const {
		return ra.GetTotalNumOfResources();
	}

private:

	/** ApplicationManager instance */
	ApplicationManagerStatusIF & am;

	/** ResourceAccounter instance */
	ResourceAccounterStatusIF & ra;

	/** Constructor */
	SystemView() :
		am(ApplicationManager::GetInstance()),
		ra(ResourceAccounter::GetInstance()) {
	}
};


} // namespace bbque

#endif  // BBQUE_SYSTEM_VIEW_H_
