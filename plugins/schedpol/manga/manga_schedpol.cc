/*
 * Copyright (C) 2016  Politecnico di Milano
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

#include "manga_schedpol.h"

#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "bbque/modules_factory.h"
#include "bbque/utils/logging/logger.h"

#include "bbque/app/working_mode.h"
#include "bbque/res/binder.h"

#define MODULE_CONFIG SCHEDULER_POLICY_CONFIG "." SCHEDULER_POLICY_NAME

#ifndef CONFIG_TARGET_LINUX_MANGO
#error "MangA policy must be compiled only for Linux Mango target"
#endif

namespace bu = bbque::utils;
namespace po = boost::program_options;

namespace bbque { namespace plugins {

// :::::::::::::::::::::: Static plugin interface ::::::::::::::::::::::::::::

void * MangASchedPol::Create(PF_ObjectParams *) {
	return new MangASchedPol();
}

int32_t MangASchedPol::Destroy(void * plugin) {
	if (!plugin)
		return -1;
	delete (MangASchedPol *)plugin;
	return 0;
}

// ::::::::::::::::::::: Scheduler policy module interface :::::::::::::::::::

char const * MangASchedPol::Name() {
	return SCHEDULER_POLICY_NAME;
}

MangASchedPol::MangASchedPol():
		cm(ConfigurationManager::GetInstance()),
		ra(ResourceAccounter::GetInstance()),
		rmv(ResourceMappingValidator::GetInstance()) {
	logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	assert(logger);
	if (logger)
		logger->Info("manga: Built a new dynamic object[%p]", this);
	else
		fprintf(stderr,
			FI("manga: Built new dynamic object [%p]\n"), (void *)this);
}


MangASchedPol::~MangASchedPol() {

}


SchedulerPolicyIF::ExitCode_t MangASchedPol::Init() {
	// Build a string path for the resource state view
	std::string token_path(MODULE_NAMESPACE);
	++status_view_count;
	token_path.append(std::to_string(status_view_count));
	logger->Debug("Init: Require a new resource state view [%s]",
		token_path.c_str());

	// Get a fresh resource status view
	ResourceAccounterStatusIF::ExitCode_t ra_result =
		ra.GetView(token_path, sched_status_view);
	if (ra_result != ResourceAccounterStatusIF::RA_SUCCESS) {
		logger->Fatal("Init: cannot get a resource state view");
		return SCHED_ERROR_VIEW;
	}
	logger->Debug("Init: resources state view token: %ld", sched_status_view);

	return SCHED_OK;
}


SchedulerPolicyIF::ExitCode_t
MangASchedPol::Schedule(
		System & system,
		RViewToken_t & status_view) {
	SchedulerPolicyIF::ExitCode_t result = SCHED_DONE;

	// Class providing query functions for applications and resources
	sys = &system;
	Init();

	for (AppPrio_t priority = 0; priority <= sys->ApplicationLowestPriority(); priority++) {
		// Checking if there are applications at this priority
		if (!sys->HasApplications(priority))
			continue;

		logger->Debug("Serving applications with priority %d", priority);

		ExitCode_t err = ServeApplicationsWithPriority(priority);
		if (err == SCHED_R_UNAVAILABLE) {
			// We have finished the resources, suspend all other apps and returns
			// gracefully
			// TODO: Suspend apps
			result = SCHED_DONE;
			break;
		} else if (err != SCHED_OK) {
			logger->Error("Unexpected error in policy scheduling: %d", err);
			result = err;
			break;
		}

	}

	// Return the new resource status view according to the new resource
	// allocation performed
	status_view = sched_status_view;
	return result;
}

SchedulerPolicyIF::ExitCode_t
MangASchedPol::ServeApplicationsWithPriority(int priority) noexcept {


	ExitCode_t err, err_relax = SCHED_OK;
	do {
		ba::AppCPtr_t  papp;
		AppsUidMapIt app_iterator;
		// Get all the applications @ this priority
		papp = sys->GetFirstWithPrio(priority, app_iterator);
		for (; papp; papp = sys->GetNextWithPrio(priority, app_iterator)) {
			err = ServeApp(papp);

			if(err == SCHED_SKIP_APP) {
				continue;
			}
			else if (err == SCHED_R_UNAVAILABLE) {
				SuspendStrictApps(priority);
				err_relax = RelaxRequirements(priority);
				break;
			}
			
			if (err != SCHED_OK) {
				// It returns SCHED_R_UNAVAILABLE if no more bandwidth is available
				// or the error exit code in case of error.
				return err;
			}
		}

	} while (err != SCHED_OK && err_relax == SCHED_OK);

	return err_relax != SCHED_OK ? err_relax : err;
}

void MangASchedPol::SuspendStrictApps(int priority) noexcept {
	//TODO
}

SchedulerPolicyIF::ExitCode_t MangASchedPol::RelaxRequirements(int priority) noexcept {
	//TODO
}

SchedulerPolicyIF::ExitCode_t MangASchedPol::ServeApp(ba::AppCPtr_t papp) noexcept {

	ResourceMappingValidator::ExitCode_t rmv_err;
	std::list<Partition> partitions;
	
	AllocateArchitectural(papp);
	
	rmv_err = rmv.LoadPartitions(*papp->GetTaskGraph(), partitions);

	switch(rmv_err) {
//		case PMV_NO_PARTITION:
//			return SCHED_OK; 	// TODO
		default:
			return SCHED_OK;	// TODO
	}

}


SchedulerPolicyIF::ExitCode_t MangASchedPol::AllocateArchitectural(ba::AppCPtr_t papp) noexcept {

}


} // namespace plugins

} // namespace bbque
