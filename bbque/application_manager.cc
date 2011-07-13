/**
 *       @file  application_manager.cc
 *      @brief  Application Manager implementation
 *
 * This implements the ApplicationManager class.
 *
 *     @author  Giuseppe Massari (jumanix), joe.massanga@gmail.com
 *
 *   @internal
 *     Created  06/04/2011
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  Politecnico di Milano
 *   Copyright  Copyright (c) 2011, Giuseppe Massari
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * ============================================================================
 */

#include "bbque/application_manager.h"

#include "bbque/configuration_manager.h"
#include "bbque/modules_factory.h"
#include "bbque/plugin_manager.h"

#include "bbque/app/application.h"
#include "bbque/app/working_mode.h"
#include "bbque/app/recipe.h"
#include "bbque/app/constraints.h"
#include "bbque/plugins/recipe_loader.h"
#include "bbque/res/resources.h"

#include "bbque/application_manager.h"
#include "bbque/res/resource_accounter.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

namespace ba = bbque::app;
namespace br = bbque::res;
namespace bp = bbque::plugins;
namespace po = boost::program_options;

namespace bbque {

ApplicationManager & ApplicationManager::GetInstance() {
	static ApplicationManager instance;
	return instance;
}


ApplicationManager::ApplicationManager() {

	// Get a logger
	bp::LoggerIF::Configuration conf(APPLICATION_MANAGER_NAMESPACE);
	logger = ModulesFactory::GetLoggerModule(std::cref(conf));
	assert(logger);

	//  Get the recipe loader instance
	rloader = ModulesFactory::GetRecipeLoaderModule();
	if (!rloader) {
		logger->Fatal("Missing RecipeLoader plugin");
		assert(rloader);
	}

	// Read the lowest application priority from configuraiton file
	ConfigurationManager & cm = ConfigurationManager::GetInstance();
	po::options_description opts_desc("Application Manager Options");
	opts_desc.add_options()
		("app.lowest_prio",
		 po::value<app::AppPrio_t>
		 (&lowest_prio)->default_value(BBQUE_APP_PRIO_MIN),
		 "Low application priority (the higher the value, "
		 "the lower the prio")
		;
	po::variables_map opts_vm;
	cm.ParseConfigurationFile(opts_desc, opts_vm);

	// Pre-allocate priority and status vectors
	priority_vec = std::vector<AppsUidMap_t>(lowest_prio + 1);

	// Debug logging
	logger->Debug("Min priority = %d", lowest_prio);
	logger->Debug("Priority vector [size: %d, capacity: %d]",
			priority_vec.size(), priority_vec.capacity());

}


ApplicationManager::~ApplicationManager() {

	// Clear the recipes
	logger->Debug("Clearing recipes...");
	recipes.clear();

	// Clear the priority vector
	logger->Debug("Clearing priority vector...");
	priority_vec.clear();

	// Clear the applications map
	logger->Debug("Clearing apps...");
	apps.clear();
	uids.clear();
}

bp::RecipeLoaderIF::ExitCode_t ApplicationManager::LoadRecipe(
		std::string const & recipe_name,
		RecipePtr_t & recipe,
		bool weak_load) {
	std::unique_lock<std::mutex> recipes_ul(recipes_mtx);
	bp::RecipeLoaderIF::ExitCode_t result;
	logger->Debug("Loading recipe [%s]...", recipe_name.c_str());

	assert(rloader);
	if (!rloader) {
		logger->Error("Cannot load recipe [%s] "
				"(Error: missing recipe loader module)",
				recipe_name.c_str());
		return bp::RecipeLoaderIF::RL_ABORTED;
	}

	//---  Checking for previously loaded recipe
	std::map<std::string, RecipePtr_t>::iterator it(
			recipes.find(recipe_name));
	if (it != recipes.end()) {
		// Return a previously loaded recipe
		logger->Debug("recipe [%s] already loaded",
				recipe_name.c_str());
		recipe = (*it).second;
		return bp::RecipeLoaderIF::RL_SUCCESS;
	}

	//---  Loading a new recipe
	logger->Info("Loading NEW recipe [%s]...", recipe_name.c_str());

	// Load the required recipe
	recipe = RecipePtr_t(new ba::Recipe(recipe_name));
	result = rloader->LoadRecipe(recipe_name, recipe);

	// If a weak load has done, but the we didn't want it,
	// or a parsing error happened: then return an empty recipe
	if (result == bp::RecipeLoaderIF::RL_WEAK_LOAD && !weak_load) {
		logger->Error("Load NEW recipe [%s] FAILED "
				"(Error: weak load not accepted)",
				recipe_name.c_str());
		recipe = RecipePtr_t();
		return result;

	}
	// On all other case just WEAK_LOAD and SUCCESS are acceptable
	if (result >= bp::RecipeLoaderIF::RL_FAILED ) {
		logger->Error("Load NEW recipe [%s] FAILED "
				"(Error: %d)",
				recipe_name.c_str(), result);
		recipe = RecipePtr_t();
		return result;
	}

	logger->Debug("recipe [%s] load DONE", recipe_name.c_str());

	// Place the new recipe object in the map, and return it
	recipes[recipe_name] = recipe;

	return bp::RecipeLoaderIF::RL_SUCCESS;

}


/*******************************************************************************
 *  Get EXC handlers
 ******************************************************************************/

AppPtr_t const ApplicationManager::GetApplication(AppUid_t uid) const {
	AppsUidMap_t::const_iterator it = uids.find(uid);
	AppPtr_t papp;

	logger->Debug("Looking for UID [%07d]...", uid);

	//----- Find the required EXC
	if (it == uids.end()) {
		logger->Error("Lookup UID [%07d] FAILED "
				"(Error: UID not registered)", uid);
		assert(it != uids.end());
		return AppPtr_t();
	}

	papp = (*it).second;
	logger->Debug("Found UID [%07d] => [%s]", uid, papp->StrId());

	return papp;
}

AppPtr_t const
ApplicationManager::GetApplication(AppPid_t pid, uint8_t exc_id) {
	logger->Debug("Looking for EXC [%05d:*:%02d]...", pid, exc_id);
	return GetApplication(Application::Uid(pid, exc_id));
}

/*******************************************************************************
 *  EXC state handling
 ******************************************************************************/

#define DOUBLE_LOCK(cur, next)\
	if (cur > next) {\
		currState_ul.lock();\
		nextState_ul.lock();\
	} else {\
		nextState_ul.lock();\
		currState_ul.lock();\
	}\


void ApplicationManager::ReportStatusQ() const {

	// Report on current status queue
	char report[] = "StateQ: [DIS: 000, RDY: 000, SYC: 000, RUN: 000, FIN: 000]";
	for (uint8_t i = 0; i < Application::STATE_COUNT; ++i) {
		::sprintf(report+14+i*10, "%03u", (unsigned)status_vec[i].size());
		report[17+i*10] = ',';
	}
	report[17+10*(Application::STATE_COUNT-1)] = ']';
	logger->Info(report);

}

void ApplicationManager::ReportSyncQ() const {

	// Report on current status queue
	char report[] = "SyncQ:  [STA: 000, REC: 000, M/R: 000, MIG: 000, BLK: 000]";
	for (uint8_t i = 0; i < Application::SYNC_STATE_COUNT; ++i) {
		::sprintf(report+14+i*10, "%03u", (unsigned)sync_vec[i].size());
		report[17+i*10] = ',';
	}
	report[17+10*(Application::SYNC_STATE_COUNT-1)] = ']';
	logger->Info(report);

}

ApplicationManager::ExitCode_t
ApplicationManager::UpdateStatusMaps(AppPtr_t papp,
		Application::State_t prev, Application::State_t next) {

	assert(papp);
	assert(prev != next);

	// Retrieve the runtime map from the status vector
	AppsUidMap_t *currStateMap = &(status_vec[prev]);
	AppsUidMap_t *nextStateMap = &(status_vec[next]);
	assert(currStateMap != nextStateMap);

	// Move it from the current to the next status map
	// FIXME: maybe we could avoid to enqueue FINISHED EXCs
	nextStateMap->insert(UidsMapEntry_t(papp->Uid(), papp));
	currStateMap->erase(papp->Uid());

	ReportStatusQ();
	ReportSyncQ();

	return AM_SUCCESS;
}

void ApplicationManager::PrintStatusReport() const {
	AppsMap_t::const_iterator app_it(apps.begin());
	AppsMap_t::const_iterator end_app(apps.end());
	char line[80];
	char curr_awm_cl[15];
	char next_awm_cl[15];

	logger->Info(
			"---- App:EXC ---|---- State |----- Sync |"
			"-- CurrentAWM |----- NextAWM |");

	for (; app_it != end_app; ++app_it) {
		ba::AwmPtr_t const & awm = app_it->second->CurrentAWM();
		ba::AwmPtr_t const & next_awm = app_it->second->NextAWM();

		if (awm)
			snprintf(curr_awm_cl, 12, "%d:%s", awm->Id(),
				awm->GetClusterSet().to_string().c_str());
		else
			snprintf(curr_awm_cl, 12, "-");

		if (next_awm)
			snprintf(next_awm_cl, 12, "%d:%s", next_awm->Id(),
				next_awm->GetClusterSet().to_string().c_str());
		else
			snprintf(next_awm_cl, 12, "-");


		snprintf(line, 80, "%12s | %9s | %9s | %12s | %12s |",
				app_it->second->StrId(),
				app_it->second->StateStr(app_it->second->State()),
				app_it->second->SyncStateStr(app_it->second->SyncState()),
				curr_awm_cl,
				next_awm_cl);

		logger->Info(line);
	}

	logger->Info(
			"----------------------------------------------------------------"
			"-------");
}

ApplicationManager::ExitCode_t
ApplicationManager::NotifyNewState(AppPtr_t papp, Application::State_t next) {
	std::unique_lock<std::mutex> currState_ul(
			status_mtx[papp->State()], std::defer_lock);
	std::unique_lock<std::mutex> nextState_ul(
			status_mtx[next], std::defer_lock);

	logger->Debug("Updating EXC [%s] state queue [%d:%s => %d:%s]",
			papp->StrId(),
			papp->State(), Application::StateStr(papp->State()),
			next, Application::StateStr(next));

	if (papp->State() == next) {
		// This should never happen
		assert(papp->State() != next);
		return AM_SUCCESS;
	}

	// Lock curr and next queue
	// FIXME: unfortunately g++ seem not yet to support the C++0x standard
	// double locking mechnism provided by std::lock(). Thus we emulate it
	// there by ensuring to acquire locks alwasy starting from the higher
	// queue to the lower one.
	//std::lock(currState_ul, nextState_ul);
	DOUBLE_LOCK(papp->State(), next);

	// If next state is not SYNC
	if (next != Application::SYNC) {
		// try to remove the app from the sync map
		SyncRemove(papp);
	} else {
		// otherwise add to the proper sync map
		SyncAdd(papp);
	}

	return UpdateStatusMaps(papp, papp->State(), next);

}


/*******************************************************************************
 *  EXC Creation
 ******************************************************************************/

AppPtr_t ApplicationManager::CreateEXC(
		std::string const & _name, AppPid_t _pid, uint8_t _exc_id,
		std::string const & _rcp_name, app::AppPrio_t _prio,
		bool _weak_load) {
	std::unique_lock<std::mutex> statusMtx_ul(
			status_mtx[Application::DISABLED], std::defer_lock);
	RecipeLoaderIF::ExitCode_t result;
	RecipePtr_t rcp_ptr;
	AppPtr_t papp;

	// Create a new descriptor
	papp = AppPtr_t(new ba::Application(_name, _pid, _exc_id));
	papp->SetPriority(_prio);

	logger->Info("Create EXC [%s], prio[%d]",
			papp->StrId(), papp->Priority());

	// Load the required recipe
	result = LoadRecipe(_rcp_name, rcp_ptr, _weak_load);
	if (!rcp_ptr) {
		logger->Error("Create EXC [%s] FAILED "
				"(Error: unable to load recipe [%s])",
				papp->StrId(), _rcp_name.c_str());
		return AppPtr_t();
	}
	papp->SetRecipe(rcp_ptr, papp);

	// Save application descriptors
	apps.insert(AppsMapEntry_t(papp->Pid(), papp));
	uids.insert(UidsMapEntry_t(papp->Uid(), papp));
	priority_vec[papp->Priority()].insert(
			UidsMapEntry_t(papp->Uid(), papp));

	// All new EXC are initially disabled
	assert(papp->State() == Application::DISABLED);
	statusMtx_ul.lock();
	status_vec[papp->State()].insert(
			UidsMapEntry_t(papp->Uid(), papp));
	statusMtx_ul.unlock();

	logger->Debug("Create EXC [%s] DONE", papp->StrId());

	return papp;
}


/*******************************************************************************
 *  EXC Destruction
 ******************************************************************************/

ApplicationManager::ExitCode_t
ApplicationManager::PriorityRemove(AppPtr_t papp) {

	logger->Debug("Releasing [%s] EXCs from PRIORITY map...",
			papp->StrId());
	priority_vec[papp->Priority()].erase(papp->Uid());

	return AM_SUCCESS;
}

ApplicationManager::ExitCode_t
ApplicationManager::StatusRemove(AppPtr_t papp) {
	std::unique_lock<std::mutex> statusMtx_ul(
			status_mtx[papp->State()]);

	logger->Debug("Releasing [%s] EXCs from STATUS map...",
			papp->StrId());
	status_vec[papp->State()].erase(papp->Uid());

	return AM_SUCCESS;
}

ApplicationManager::ExitCode_t
ApplicationManager::AppsRemove(AppPtr_t papp) {
	std::pair<AppsMap_t::iterator, AppsMap_t::iterator> range;
	AppsMap_t::iterator it;

	logger->Debug("Releasing [%s] EXC from APPs map...",
			papp->StrId());
	range = apps.equal_range(papp->Pid());
	it = range.first;
	while (it != range.second &&
		((*it).second)->ExcId() != papp->ExcId()) {
		++it;
	}
	assert(it != range.second);
	if (it == range.second) {
		logger->Crit("EXCs [%s] not found in application map "
				"(Error: possible data structure corruption)",
			papp->StrId());
		return AM_DATA_CORRUPT;
	}
	apps.erase(it);

	return AM_SUCCESS;
}

ApplicationManager::ExitCode_t
ApplicationManager::DestroyEXC(AppPtr_t papp) {
	ApplicationManager &am(ApplicationManager::GetInstance());
	br::ResourceAccounter &ra(br::ResourceAccounter::GetInstance());
	ExitCode_t result;

	logger->Debug("Removing EXC [%s] ...", papp->StrId());

	// Mark the EXC as finished
	papp->Terminate();

	// Remove execution context form priority and status maps
	result = PriorityRemove(papp);
	if (result != AM_SUCCESS)
		return result;

	result = StatusRemove(papp);
	if (result != AM_SUCCESS)
		return result;

	result = AppsRemove(papp);
	if (result != AM_SUCCESS)
		return result;

	logger->Debug("Releasing [%s] EXC from UIDs map...",
			papp->StrId());

	uids.erase(papp->Uid());

	logger->Info("EXC Released [%s]", papp->StrId());
	ReportStatusQ();
	ReportSyncQ();
	am.PrintStatusReport();
	ra.PrintStatusReport();

	return AM_SUCCESS;
}

ApplicationManager::ExitCode_t
ApplicationManager::DestroyEXC(AppPid_t pid, uint8_t exc_id) {
	AppPtr_t papp;

	// Find the required EXC
	papp = GetApplication(Application::Uid(pid, exc_id));
	assert(papp);
	if (!papp) {
		logger->Warn("Stop EXC [%d:*:%d] FAILED "
				"(Error: EXC not found)");
		return AM_EXC_NOT_FOUND;
	}

	return DestroyEXC(papp);
}

ApplicationManager::ExitCode_t
ApplicationManager::DestroyEXC(AppPid_t pid) {
	std::pair<AppsMap_t::iterator, AppsMap_t::iterator> range;
	AppsMap_t::iterator it;
	ExitCode_t result;
	AppPtr_t papp;

	range = apps.equal_range(pid);
	it = range.first;
	for ( ; it != range.second; ++it) {
		papp = (*it).second;
		result = DestroyEXC(papp);
		if (result != AM_SUCCESS)
			return result;
	}

	return AM_SUCCESS;
}


/*******************************************************************************
 *  EXC Enabling
 ******************************************************************************/

ApplicationManager::ExitCode_t
ApplicationManager::EnableEXC(AppPtr_t papp) {

	// Enabling the execution context
	logger->Debug("Enabling EXC [%s]...", papp->StrId());

	if (papp->Enable() != Application::APP_SUCCESS) {
		return AM_ABORT;
	}

	return AM_SUCCESS;
}

ApplicationManager::ExitCode_t
ApplicationManager::EnableEXC(AppPid_t pid, uint8_t exc_id) {
	AppPtr_t papp;

	// Find the required EXC
	papp = GetApplication(Application::Uid(pid, exc_id));
	if (!papp) {
		logger->Warn("Enable EXC [%d:*:%d] FAILED "
				"(Error: EXC not found)");
		assert(papp);
		return AM_EXC_NOT_FOUND;
	}

	return EnableEXC(papp);

}


/*******************************************************************************
 *  EXC Disabling
 ******************************************************************************/

ApplicationManager::ExitCode_t
ApplicationManager::DisableEXC(AppPtr_t papp) {
	// A disabled EXC is moved (as soon as possible) into the DISABLED queue
	// NOTE: other code-path should check wheter an application is still
	// !DISABLED to _assume_ a normal operation

	logger->Debug("Disabling EXC [%s]...", papp->StrId());

	if (papp->Disable() != Application::APP_SUCCESS) {
		return AM_ABORT;
	}

	return AM_SUCCESS;
}

ApplicationManager::ExitCode_t
ApplicationManager::DisableEXC(AppPid_t pid, uint8_t exc_id) {
	AppPtr_t papp;

	// Find the required EXC
	papp = GetApplication(Application::Uid(pid, exc_id));
	if (!papp) {
		logger->Warn("Disable EXC [%d:*:%d] FAILED "
				"(Error: EXC not found)");
		assert(papp);
		return AM_ABORT;
	}

	return DisableEXC(papp);
}


/*******************************************************************************
 *  EXC Synchronization
 ******************************************************************************/

void
ApplicationManager::SyncRemove(AppPtr_t papp, Application::SyncState_t state) {
	std::unique_lock<std::mutex> sync_ul(sync_mtx[state]);

	assert(papp);

	// Get the applications map
	if (sync_vec[state].erase(papp->Uid())) {
		logger->Info("Removed sync request for EXC [%s, %s]",
				papp->StrId(),
				papp->SyncStateStr());
		return;
	}

	// We should never got there
	assert(false);

}

void
ApplicationManager::SyncRemove(AppPtr_t papp) {

	assert(papp);
	logger->Debug("Removing sync request for EXC [%s]...", papp->StrId());

	// Disregard EXCs which are not in SYNC state
	if (!papp->Synching())
		return;

	SyncRemove(papp, papp->SyncState());

}

void
ApplicationManager::SyncAdd(AppPtr_t papp, Application::SyncState_t state) {
	std::unique_lock<std::mutex> sync_ul(sync_mtx[state]);
	assert(papp);
	sync_vec[state].insert(UidsMapEntry_t(papp->Uid(), papp));
}

void
ApplicationManager::SyncAdd(AppPtr_t papp) {

	assert(papp);

	SyncAdd(papp, papp->SyncState());

	logger->Info("Added sync request for EXC [%s, %d:%s]",
				papp->StrId(),
				papp->SyncState(),
				papp->SyncStateStr());

}

ApplicationManager::ExitCode_t
ApplicationManager::SyncRequest(AppPtr_t papp, Application::SyncState_t state) {

	logger->Debug("Requesting sync for EXC [%s, %s]", papp->StrId(),
			Application::SyncStateStr(state));

	// The state at this point should be SYNC
	if (!papp->Synching()) {
		logger->Crit("Sync request for EXC [%s] FAILED "
				"(Error: invalid EXC state [%d]",
				papp->StrId(), papp->State());
		assert(papp->Synching());
		return AM_ABORT;
	}

	// Check valid state has beed required
	if (state >= Application::SYNC_STATE_COUNT) {
		logger->Crit("Sync request for EXC [%s] FAILED "
				"(Error: invalid sync state required [%d]",
				papp->StrId(), state);
		assert(state < Application::SYNC_STATE_COUNT);
		return AM_ABORT;
	}

	// TODO notify the Resource Manager

	logger->Info("Sync request for EXC [%s, %s]", papp->StrId(),
			Application::SyncStateStr(state));

	return AM_SUCCESS;
}

ApplicationManager::ExitCode_t
ApplicationManager::SyncCommit(AppPtr_t papp) {
	Application::SyncState_t syncState = papp->SyncState();

	logger->Info("Synching EXC [%s, %s]...", papp->StrId(),
			papp->SyncStateStr());
	assert(syncState != Application::SYNC_NONE);

	// Notify application
	papp->ScheduleCommit();

	logger->Info("Sync for EXC [%s, %s] DONE", papp->StrId(),
			papp->SyncStateStr());

	PrintStatusReport();

	return AM_SUCCESS;
}

}   // namespace bbque

