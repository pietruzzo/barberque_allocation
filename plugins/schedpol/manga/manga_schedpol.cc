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
#include "bbque/utils/assert.h"
#include "bbque/app/working_mode.h"
#include "bbque/res/binder.h"
#include "bbque/tg/task_graph.h"

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

			logger->Debug("Trying to allocate resources for application %s [pid=%d]", 
					papp->Name().c_str(), papp->Pid());

			// Try to allocate resourced for the application
			err = ServeApp(papp);

			if(err == SCHED_SKIP_APP) {
				// In this case we have no sufficient memory to start it, the only
				// one thing to do is to ignore it
				logger->Notice("Unable to find resource for application %s [pid=%d]", 
					papp->Name().c_str(), papp->Pid());
				continue;
			}

			else if (err == SCHED_R_UNAVAILABLE) {
				// In this case we have no bandwidth feasibility, so we can try to
				// fairly reduce the bandwidth for non strict applications.
				err_relax = RelaxRequirements(priority);
				break;
			}
			
			else if (err != SCHED_OK) {
				// It returns  error exit code in case of error.
				return err;
			}

			logger->Info("Application %s [pid=%d] allocated successfully",
				     papp->Name().c_str(), papp->Pid());
		}

	} while (err != SCHED_OK && err_relax == SCHED_OK);

	return err_relax != SCHED_OK ? err_relax : err;
}

SchedulerPolicyIF::ExitCode_t MangASchedPol::RelaxRequirements(int priority) noexcept {
	//TODO: smart policy to reduce the requirements

	UNUSED(priority);

	return SCHED_R_UNAVAILABLE;
}

SchedulerPolicyIF::ExitCode_t MangASchedPol::ServeApp(ba::AppCPtr_t papp) noexcept {

	SchedulerPolicyIF::ExitCode_t err;
	ResourceMappingValidator::ExitCode_t rmv_err;
	std::list<Partition> partitions;
	
	// First of all we have to decide which processor type to assign to each task
	err = AllocateArchitectural(papp);
	
	if (err != SCHED_OK) {
		logger->Error("Allocate architectural failed");
		return err;
	}

	rmv_err = rmv.LoadPartitions(*papp->GetTaskGraph(), partitions);

	switch(rmv_err) {
		case ResourceMappingValidator::PMV_OK:
			logger->Debug("LoadPartitions SUCCESS");
			return SCHED_OK;
		case ResourceMappingValidator::PMV_SKIMMER_FAIL:
			logger->Error("At least one skimmer failed unexpectly");
			return SCHED_ERROR;
		case ResourceMappingValidator::PMV_NO_PARTITION:
			logger->Debug("LoadPartitions NO PARTITION");
			return DealWithNoPartitionFound(papp);
		default:
			logger->Fatal("Unexpected LoadPartitions return (?)");
			// Ehi what's happened here?
			return SCHED_ERROR;
	}

}

SchedulerPolicyIF::ExitCode_t MangASchedPol::DealWithNoPartitionFound(ba::AppCPtr_t papp) noexcept {

	UNUSED(papp);

	switch(rmv.GetLastFailed()) {

		// In this case we can try to reduce the allocated bandwidth
		case PartitionSkimmer::SKT_MANGO_HN:
			return SCHED_R_UNAVAILABLE;

		// Strict thermal constraints must not violated
		case PartitionSkimmer::SKT_MANGO_POWER_MANAGER:	
		// We have no sufficient memory to run the app
		case PartitionSkimmer::SKT_MANGO_MEMORY_MANAGER:
		default:
			return SCHED_SKIP_APP;
	}

}

SchedulerPolicyIF::ExitCode_t MangASchedPol::AllocateArchitectural(ba::AppCPtr_t papp) noexcept {

	// Trivial allocation policy: we select always the best one for the receipe
	// TODO: a smart one

	if (nullptr == papp->GetTaskGraph()) {
		logger->Error("TaskGraph not present for application %s [pid=%d]",
				papp->Name().c_str(), papp->Pid());
		return SCHED_ERROR;
	}

	for (auto task_pair : papp->GetTaskGraph()->Tasks()) {
		auto task = task_pair.second;
		const auto requirements = papp->GetTaskRequirements(task->Id());
	
		uint_fast8_t i=0; 
		ArchType_t preferred_type;
		const auto targets = task->Targets();
		do {	// Select every time the best preferred available architecture
			if ( i > 0 ) {
				logger->Warn("I wanted to select architecture %d available in "
					     "receipe but the task %i does not support it",
					     preferred_type, task->Id() );
			}
			preferred_type = requirements.ArchPreference(i);
			i++;
		} while(targets.find(preferred_type) == targets.end());

		if (preferred_type == ArchType_t::NONE) {
			logger->Error("No architecture available for task %d", task->Id());
			return SCHED_SKIP_APP;
		}

		logger->Info("Task %d preliminary assignment [arch=%d, in_bw=%d, out_bw=%d]",
			task->Id(), preferred_type, requirements.GetAssignedBandwidth().in_kbps,
			requirements.GetAssignedBandwidth().out_kbps);
		task->SetAssignedArch( preferred_type );
		task->SetAssignedBandwidth( requirements.GetAssignedBandwidth() );
	}
	return SCHED_OK;

}

SchedulerPolicyIF::ExitCode_t
MangASchedPol::SelectTheBestPartition(ba::AppCPtr_t papp, const std::list<Partition> &partitions) noexcept {

	bbque_assert(partitions.size() > 0);

	// TODO: Intelligent policy

	// For the demo just select the first partition
	rmv.PropagatePartition(*papp->GetTaskGraph(), partitions.front());

	return SCHED_OK;

}

} // namespace plugins

} // namespace bbque
