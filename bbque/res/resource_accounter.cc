/**
 *       @file  resource_accounter.cc
 *      @brief  Implementation of the Resource Accounter component
 *
 * This implements the componter for making resource accounting.
 *
 * Each resource of system/platform should be properly registered in the
 * Resource accounter. It keeps track of the information upon availability,
 * total amount and used resources.
 * The information above are updated through proper methods which must be
 * called when an application working mode has triggered.
 *
 *     @author  Giuseppe Massari (jumanix), joe.massanga@gmail.com
 *
 *   @internal
 *     Created  04/04/2011
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  Politecnico di Milano
 *   Copyright  Copyright (c) 2011, Giuseppe Massari
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * ============================================================================
 */

#include "bbque/res/resource_accounter.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <locale>
#include <memory>
#include <map>
#include <string>
#include <sstream>

#include "bbque/system_view.h"
#include "bbque/modules_factory.h"
#include "bbque/plugin_manager.h"
#include "bbque/platform_services.h"
#include "bbque/app/application.h"
#include "bbque/app/working_mode.h"

namespace bbque { namespace res {

ResourceAccounter & ResourceAccounter::GetInstance() {
	static ResourceAccounter instance;
	return instance;
}

ResourceAccounter::ResourceAccounter() :
	am(ApplicationManager::GetInstance()) {

	// Get a logger
	std::string logger_name(RESOURCE_ACCOUNTER_NAMESPACE);
	plugins::LoggerIF::Configuration conf(logger_name.c_str());
	logger = ModulesFactory::GetLoggerModule(std::cref(conf));
	assert(logger);

	// Init the system resources state view
	sys_usages_view = AppUsagesMapPtr_t(new AppUsagesMap_t);
	sys_view_token = 0;
	usages_per_views[sys_view_token] = sys_usages_view;
	rsrc_per_views[sys_view_token] = ResourceSetPtr_t(new ResourceSet_t);

	// Init sync session info
	sync_ssn.count = 0;
}

ResourceAccounter::~ResourceAccounter() {
	resources.clear();
	usages_per_views.clear();
	rsrc_per_views.clear();
}

uint16_t ResourceAccounter::ClusteringFactor(std::string const & path) {
	uint16_t clustering_factor;

	// Check if the resource exists
	if (!ExistResource(path))
		return 0;

	// Check if the resource is clustered
	int16_t clust_patt_pos = path.find(RSRC_CLUSTER);
	if (clust_patt_pos < 0)
		return 1;

	// Compute the clustering factor
	clustering_factor = Total(RSRC_CLUSTER);
	if (clustering_factor == 0)
		++clustering_factor;

	return clustering_factor;
}

ResourceAccounter::ExitCode_t ResourceAccounter::RegisterResource(
		std::string const & _path,
		std::string const & _units,
		uint64_t _amount) {
	// Check arguments
	if(_path.empty()) {
		logger->Fatal("Registering: Invalid resource path");
		return RA_ERR_MISS_PATH;
	}

	// Insert a new resource in the tree
	ResourcePtr_t res_ptr = resources.insert(_path);
	if (!res_ptr) {
		logger->Crit("Registering: Unable to allocate a new resource"
				"descriptor");
		return RA_ERR_MEM;
	}

	// Set the amount of resource considering the units
	res_ptr->SetTotal(ConvertValue(_amount, _units));

	// Insert the path in the paths set
	paths.insert(_path);
	path_max_len = std::max((int) path_max_len, (int) _path.length());

	return RA_SUCCESS;
}

void ResourceAccounter::PrintStatusReport(RViewToken_t vtok) const {
	std::set<std::string>::const_iterator path_it(paths.begin());
	std::set<std::string>::const_iterator end_path(paths.end());
	char padded_path[50];
	char pad[30];

	logger->Info("Report on state view: %d", vtok);
	logger->Info(
			"------------- Resources --------------- Used ------ Total -");
	for (; path_it != end_path; ++path_it) {
		memset(pad, ' ', path_max_len + 8 - (*path_it).length());
		snprintf(padded_path, path_max_len + 8, "%s%s",
				(*path_it).c_str(), pad);

		logger->Info("%s : %10llu | %10llu |",
				padded_path, Used(*path_it, vtok), Total(*path_it));
	}

	logger->Info(
			"-----------------------------------------------------------");
}

uint64_t ResourceAccounter::QueryStatus(ResourcePtrList_t const & rsrc_set,
		QueryOption_t _att,
		RViewToken_t vtok) const {
	// Cumulative value to return
	uint64_t val = 0;

	// For all the descriptors in the list add the quantity of resource in the
	// specified state (available, used, total)
	ResourcePtrList_t::const_iterator res_it(rsrc_set.begin());
	ResourcePtrList_t::const_iterator res_end(rsrc_set.end());
	for (; res_it != res_end; ++res_it) {

		switch(_att) {
		// Resource availability
		case RA_AVAIL:
			val += (*res_it)->Availability(vtok);
			break;
		// Resource used
		case RA_USED:
			val += (*res_it)->Used(vtok);
			break;
		// Resource total
		case RA_TOTAL:
			val += (*res_it)->Total();
			break;
		}
	}
	return val;
}

ResourceAccounter::ExitCode_t ResourceAccounter::BookResources(AppPtr_t papp,
		UsagesMapPtr_t const & resource_set,
		RViewToken_t vtok,
		bool do_check) {
	std::unique_lock<std::recursive_mutex> status_ul(status_mtx);

	// Check to avoid null pointer segmentation fault
	if (!papp) {
		logger->Fatal("Booking: Null pointer to the application descriptor");
		return RA_ERR_MISS_APP;
	}
	// Check that the next working mode has been set
	if ((!resource_set) || (resource_set->empty())) {
		logger->Fatal("Booking: Empty resource usages set");
		return RA_ERR_MISS_USAGES;
	}

	// Get the map of resources used by the application (from the state view
	// referenced by 'vtok'). A missing view implies that the token is not
	// valid.
	AppUsagesMapPtr_t apps_usages;
	if (GetAppUsagesByView(vtok, apps_usages) == RA_ERR_MISS_VIEW) {
		logger->Fatal("Booking: Invalid resource state view token");
		return RA_ERR_MISS_VIEW;
	}

	// Each application can hold just one resource usages set
	AppUsagesMap_t::iterator usemap_it(apps_usages->find(papp->Uid()));
	if (usemap_it != apps_usages->end()) {
		logger->Debug("Booking: [%s] currently using a resource set yet",
				papp->StrId());
		return RA_ERR_APP_USAGES;
	}

	// Check resource availability (if this is not a sync session)
	if ((do_check) && !(sync_ssn.started)) {
		if (CheckAvailability(resource_set, vtok) == RA_ERR_USAGE_EXC) {
			logger->Debug("Booking: Cannot allocate the resource set");
			return RA_ERR_USAGE_EXC;
		}
	}

	// Increment the booking counts and save the reference to the resource set
	// used by the application
	IncBookingCounts(resource_set, papp, vtok);
	apps_usages->insert(std::pair<AppUid_t, UsagesMapPtr_t>(papp->Uid(),
				resource_set));
	logger->Debug("Booking: [%s] now holds %d resources", papp->StrId(),
			resource_set->size());

	return RA_SUCCESS;
}

void ResourceAccounter::ReleaseResources(AppPtr_t papp, RViewToken_t vtok) {
	std::unique_lock<std::recursive_mutex> status_ul(status_mtx);

	// Check to avoid null pointer seg-fault
	if (!papp) {
		logger->Fatal("Release: Null pointer to the application descriptor");
		return;
	}

	// Get the map of applications resource usages related to the state view
	// referenced by 'vtok'
	AppUsagesMapPtr_t apps_usages;
	if (GetAppUsagesByView(vtok, apps_usages) == RA_ERR_MISS_VIEW) {
		logger->Fatal("Release: Resource view unavailable");
		return;
	}

	// Get the map of resource usages of the application
	AppUsagesMap_t::iterator usemap_it(apps_usages->find(papp->Uid()));
	if (usemap_it == apps_usages->end()) {
		logger->Fatal("Release: Application referenced misses a resource set."
				" Possible data corruption occurred.");
		return;
	}

	// Decrement resources counts and remove the usages map
	DecBookingCounts(usemap_it->second, papp, vtok);
	apps_usages->erase(papp->Uid());
	logger->Debug("Release: [%s] resource release terminated", papp->StrId());

	// Release resources from sync view
	if ((sync_ssn.started) && (papp->Active()) && (vtok != sync_ssn.view))
		ReleaseResources(papp, sync_ssn.view);
}

ResourceAccounter::ExitCode_t ResourceAccounter::CheckAvailability(
		UsagesMapPtr_t const & usages,
		RViewToken_t vtok) const {
	UsagesMap_t::const_iterator usages_it(usages->begin());
	UsagesMap_t::const_iterator usages_end(usages->end());

	// Check availability for each ResourceUsage object
	for (; usages_it != usages_end; ++usages_it) {
		uint64_t avail = Available(usages_it->second, vtok);
		if (avail < usages_it->second->value) {
			logger->Debug("ChkAvail: Exceeding request for {%s}"
					"[USG:%llu | AV:%llu | TOT:%llu] ",
					usages_it->first.c_str(),
					usages_it->second->value,
					avail,
					Total(usages_it->second));
			return RA_ERR_USAGE_EXC;
		}
	}
	return RA_SUCCESS;
}

ResourceAccounter::ExitCode_t ResourceAccounter::GetView(std::string req_path,
		RViewToken_t & token) {
	// Null-string check
	if (req_path.empty()) {
		logger->Error("GetView: Missing a valid string");
		return RA_ERR_MISS_PATH;
	}

	// Token
	token = std::hash<std::string>()(req_path);
	logger->Debug("GetView: New resource state view. Token = %d", token);

	// Allocate a new view for the applications resource usages and the
	// set fo resources allocated
	usages_per_views.insert(std::pair<RViewToken_t, AppUsagesMapPtr_t>(token,
				AppUsagesMapPtr_t(new AppUsagesMap_t)));
	rsrc_per_views.insert(std::pair<RViewToken_t, ResourceSetPtr_t>(token,
				ResourceSetPtr_t(new ResourceSet_t)));

	return RA_SUCCESS;
}

void ResourceAccounter::PutView(RViewToken_t vtok) {
	// Do nothing if the token references the system state view
	if (vtok == sys_view_token) {
		logger->Warn("PutView: Cannot release the system resources view");
		return;
	}

	// Get the resource set using the referenced view
	ResourceViewsMap_t::iterator rviews_it(rsrc_per_views.find(vtok));
	if (rviews_it == rsrc_per_views.end()) {
		logger->Error("PutView: Cannot find the resource view referenced by"
				"%d", vtok);
		return;
	}

	// For each resource delete the view
	ResourceSet_t::iterator rsrc_set_it(rviews_it->second->begin());
	ResourceSet_t::iterator rsrc_set_end(rviews_it->second->end());
	for (; rsrc_set_it != rsrc_set_end; ++rsrc_set_it)
		(*rsrc_set_it)->DeleteView(vtok);

	// Remove the map of applications resource usages of this view
	usages_per_views.erase(vtok);
	logger->Debug("PutView: view %d cleared", vtok);
}

RViewToken_t ResourceAccounter::SetView(RViewToken_t vtok) {
	// Do nothing if the token references the system state view
	if (vtok == sys_view_token) {
		logger->Warn("SetView: View %d is the system state view yet!", vtok);
		return sys_view_token;
	}

	// Set the system state view pointer to the map of applications resource
	// usages of this view and point to
	AppUsagesViewsMap_t::iterator us_view_it(usages_per_views.find(vtok));
	if (us_view_it == usages_per_views.end()) {
		logger->Fatal("SetView: View %d unknown", vtok);
		return sys_view_token;
	}

	sys_usages_view = us_view_it->second;
	sys_view_token = vtok;

	// Get the resource set using the referenced view
	ResourceViewsMap_t::iterator rviews_it(rsrc_per_views.find(vtok));
	if (rviews_it == rsrc_per_views.end()) {
		logger->Warn("SetView: No resources used in view %d", vtok);
		return sys_view_token;
	}

	// For each resource set the view as default
	ResourceSet_t::iterator rsrc_set_it(rviews_it->second->begin());
	ResourceSet_t::iterator rsrc_set_end(rviews_it->second->end());
	for (; rsrc_set_it != rsrc_set_end; ++rsrc_set_it)
		(*rsrc_set_it)->SetAsDefaultView(vtok);

	logger->Info("SetView: View %d is the new system state view",
			sys_view_token);
	return sys_view_token;
}

ResourceAccounter::ExitCode_t ResourceAccounter::SyncStart() {
	logger->Info("SyncMode: Start");
	std::unique_lock<std::mutex>(sync_ssn.mtx);
	// If the counter has reached the maximum, reset
	if (sync_ssn.count == std::numeric_limits<uint32_t>::max()) {
		logger->Debug("SyncMode: Session counter reset");
		sync_ssn.count = 0;
	}

	// Build the path for getting the resource view token
	char token_path[TOKEN_PATH_MAX_LEN];
	snprintf(token_path, TOKEN_PATH_MAX_LEN, SYNC_RVIEW_PATH"%d",
			++sync_ssn.count);
	logger->Debug("SyncMode [%d]: Requiring resource state view for %s",
			sync_ssn.count,	token_path);

	// Set the flag and get the resources sync view
	sync_ssn.started = true;
	if (GetView(token_path, sync_ssn.view) != RA_SUCCESS) {
		logger->Fatal("SyncMode [%d]: Cannot get a resource state view",
				sync_ssn.count);
		SyncFinalize();
		return RA_ERR_SYNC_VIEW;
	}

	logger->Debug("SyncMode [%d]: Resource state view token = %d", sync_ssn.count,
			sync_ssn.view);

	// Init the view with the resource accounting of running applications
	return SyncInit();
}

ResourceAccounter::ExitCode_t ResourceAccounter::SyncInit() {
	ResourceAccounter::ExitCode_t result;
	ApplicationManager::ExitCode_t am_result;
	SystemView & sv(SystemView::GetInstance());
	AppsUidMap_t::const_iterator rapp_it(sv.ApplicationsRunning()->begin());
	AppsUidMap_t::const_iterator end_rapp(sv.ApplicationsRunning()->end());

	// Running Applications/ExC
	for (; rapp_it != end_rapp; ++rapp_it) {
		AppPtr_t const & papp = rapp_it->second;

		// Application/EXC must always have a next AWM here
		if (!papp->NextAWM()) {
			assert(papp->NextAWM());
			logger->Fatal("SyncInit: [%s] missing next AWM.");
			return RA_ERR_SYNC_INIT;
		}

		logger->Debug("SyncInit: [%s] AWM {curr = %d / next = %d}",
				papp->StrId(),
				papp->CurrentAWM()->Id(),
				papp->NextAWM()->Id());

		// Acquire the resources
		result = BookResources(papp, papp->NextAWM()->GetResourceBinding(),
						sync_ssn.view, false);
		if (result != RA_SUCCESS) {
			logger->Fatal("SyncInit [%d]: Resource booking failed for %s."
					" Aborting sync session...",
					sync_ssn.count,
					papp->StrId());

			SyncAbort();
			return RA_ERR_SYNC_INIT;
		}

		// Continue to run...
		am_result = am.RunningCommit(papp);
		if (am_result != ApplicationManager::AM_SUCCESS)
			return RA_ERR_SYNC_INIT;
	}

	logger->Debug("SyncMode [%d]: Initialization finished", sync_ssn.count);
	return RA_SUCCESS;
}

ResourceAccounter::ExitCode_t ResourceAccounter::SyncAcquireResources(
		AppPtr_t const & papp, UsagesMapPtr_t const & usages) {
	// Check that we are in a synchronized session
	if (!sync_ssn.started) {
		logger->Error("SyncMode [%d]: Session not open", sync_ssn.count);
		return RA_ERR_SYNC_START;
	}

	return BookResources(papp, usages, sync_ssn.view, false);
}

void ResourceAccounter::SyncAbort() {
	PutView(sync_ssn.view);
	SyncFinalize();
	logger->Info("SyncMode [%d]: Session aborted", sync_ssn.count);
}

ResourceAccounter::ExitCode_t ResourceAccounter::SyncCommit() {
	ResourceAccounter::ExitCode_t result = RA_SUCCESS;
	if (SetView(sync_ssn.view) != sync_ssn.view) {
		logger->Fatal("SyncMode [%d]: Unable to set the new system resource"
				"state view", sync_ssn.count);
		result = RA_ERR_SYNC_VIEW;
	}

	SyncFinalize();
	if (result == RA_SUCCESS)
		logger->Info("SyncMode [%d]: Session committed", sync_ssn.count);

	PrintStatusReport();

	return result;
}

ResourceAccounter::ExitCode_t ResourceAccounter::GetAppUsagesByView(
		RViewToken_t vtok,
		AppUsagesMapPtr_t & apps_usages) {
	AppUsagesViewsMap_t::iterator view;
	if (vtok != 0) {
		// "Alternate" view case
		view = usages_per_views.find(vtok);
		if (view == usages_per_views.end()) {
			logger->Error("Application usages:"
					"Cannot find the resource state view referenced by %d",
					vtok);
			return RA_ERR_MISS_VIEW;
		}
		apps_usages = view->second;
	}
	else {
		// Default view / System state case
		assert(sys_usages_view);
		apps_usages = sys_usages_view;
	}

	return RA_SUCCESS;
}

void ResourceAccounter::IncBookingCounts(UsagesMapPtr_t const & app_usages,
		AppPtr_t const & papp,
		RViewToken_t vtok) {
	ExitCode_t result;
	// Get the set of resources referenced in the view
	ResourceViewsMap_t::iterator rviews_it(rsrc_per_views.find(vtok));
	assert(rviews_it != rsrc_per_views.end());

	// Book resources for the application
	UsagesMap_t::const_iterator usages_it(app_usages->begin());
	UsagesMap_t::const_iterator usages_end(app_usages->end());
	for (; usages_it != usages_end;	++usages_it) {
		UsagePtr_t rsrc_usage(usages_it->second);

		logger->Debug("Booking: [%s] requires resource {%s}",
				papp->StrId(),
				usages_it->first.c_str());

		// Do booking for this resource (usages_it->second)
		result = DoResourceBooking(papp, rsrc_usage, vtok, rviews_it->second);
		if (result != RA_SUCCESS)  {
			logger->Crit("Booking: unexpected fail! [USG:%llu | AV:%llu | TOT:%llu]",
				rsrc_usage->value,
				Available(usages_it->first),
				Total(usages_it->first));
			PrintStatusReport();
			assert(result == RA_SUCCESS);
		}

		logger->Debug("Booking: success [USG:%llu | AV:%llu | TOT:%llu]",
				rsrc_usage->value,
				Available(usages_it->first, vtok),
				Total(usages_it->first));
	}
}

ResourceAccounter::ExitCode_t ResourceAccounter::DoResourceBooking(
		AppPtr_t const & papp,
		UsagePtr_t & rsrc_usage,
		RViewToken_t vtok,
		ResourceSetPtr_t & rsrcs_per_view) {
	// Amount of resource to book
	uint64_t usage_value = rsrc_usage->value;

	// Get the list of resource binds
	ResourcePtrList_t::iterator it_bind(rsrc_usage->binds.begin());
	ResourcePtrList_t::iterator end_it(rsrc_usage->binds.end());
	while ((usage_value > 0) && (it_bind != end_it)) {
		// If the current resource bind has enough availability, reserve the
		// whole quantity requested here. Otherwise split it in more "sibling"
		// resource binds.
		uint64_t availab = (*it_bind)->Availability(vtok);
		if (usage_value < availab)
			usage_value -= (*it_bind)->Acquire(usage_value, papp, vtok);
		else
			usage_value -=
				(*it_bind)->Acquire(availab, papp, vtok);

		// Add the resource to the set of resources used in the view
		// referenced by 'vtok'
		rsrcs_per_view->insert(*it_bind);

		++it_bind;
	}

	// Critical error. This means that the availability of resources
	// mismatches the one checked in the scheduling phase
	if (usage_value != 0)
		return RA_ERR_USAGE_EXC;

	return RA_SUCCESS;
}

void ResourceAccounter::DecBookingCounts(UsagesMapPtr_t const & app_usages,
		AppPtr_t const & papp,
		RViewToken_t vtok) {
	// Get the set of resources referenced in the view
	ResourceViewsMap_t::iterator rviews_it(rsrc_per_views.find(vtok));
	assert(rviews_it != rsrc_per_views.end());

	// Release the amount of resource hold by each application
	UsagesMap_t::const_iterator usages_it(app_usages->begin());
	UsagesMap_t::const_iterator usages_end(app_usages->end());
	logger->Debug("DecCount: [%s] holds %d resources", papp->StrId(),
			app_usages->size());

	for (; usages_it != usages_end; ++usages_it) {
		UsagePtr_t rsrc_usage(usages_it->second);

		// Undo booking for this resource (usages_it->second)
		UndoResourceBooking(papp, rsrc_usage, vtok, rviews_it->second);
		logger->Debug("DecCount: [%s] has freed {%s} of %llu", papp->StrId(),
				usages_it->first.c_str(),
				rsrc_usage->value);
	}
}

void ResourceAccounter::UndoResourceBooking(AppPtr_t const & papp,
		UsagePtr_t & rsrc_usage,
		RViewToken_t vtok,
		ResourceSetPtr_t & rsrcs_per_view) {
	// Keep track of the amount of resource freed
	uint64_t usage_freed = 0;

	// For each resource bind release the quantity held
	ResourcePtrList_t::iterator it_bind(rsrc_usage->binds.begin());
	ResourcePtrList_t::iterator end_it(rsrc_usage->binds.end());
	while (usage_freed < rsrc_usage->value) {
		assert(it_bind != end_it);
		usage_freed += (*it_bind)->Release(papp, vtok);

		// If there are no more applications using the resource remove it
		// from the set of resources referenced in the view
		if ((*it_bind)->ApplicationsCount() == 0)
			rsrcs_per_view->erase(*it_bind);

		++it_bind;
	}
	assert(usage_freed == rsrc_usage->value);
}


}   // namespace res

}   // namespace bbque

