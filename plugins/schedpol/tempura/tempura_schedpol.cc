/*
 * Copyright (C) 2015  Politecnico di Milano
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

#include "tempura_schedpol.h"

#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "bbque/modules_factory.h"
#include "bbque/utils/logging/logger.h"

#include "bbque/app/working_mode.h"
#include "bbque/res/binder.h"

#define MODULE_CONFIG SCHEDULER_POLICY_CONFIG "." SCHEDULER_POLICY_NAME

namespace bu = bbque::utils;
namespace po = boost::program_options;

namespace bbque { namespace plugins {

// :::::::::::::::::::::: Static plugin interface ::::::::::::::::::::::::::::

void * TempuraSchedPol::Create(PF_ObjectParams *) {
	return new TempuraSchedPol();
}

int32_t TempuraSchedPol::Destroy(void * plugin) {
	if (!plugin)
		return -1;
	delete (TempuraSchedPol *)plugin;
	return 0;
}

// ::::::::::::::::::::: Scheduler policy module interface :::::::::::::::::::

char const * TempuraSchedPol::Name() {
	return SCHEDULER_POLICY_NAME;
}

TempuraSchedPol::TempuraSchedPol():
		cm(ConfigurationManager::GetInstance()),
		ra(ResourceAccounter::GetInstance()) {

	// Logger instance
	logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	assert(logger);

	if (logger)
		logger->Info("tempura: Built a new dynamic object[%p]", this);
	else
		fprintf(stderr,
				FI("tempura: Built new dynamic object [%p]\n"), (void *)this);
}


TempuraSchedPol::~TempuraSchedPol() {

}

TempuraSchedPol::ExitCode_t TempuraSchedPol::Init() {
	ResourceAccounterStatusIF::ExitCode_t ra_result;
	ExitCode_t result = OK;

	// Build a string path for the resource state view
	char token_path[30];
	++status_view_count;
	snprintf(token_path, strlen(token_path), "%s%d",
			MODULE_NAMESPACE, status_view_count);

	// Get a fresh resource status view
	logger->Debug("Init: Require a new resource state view [%s]", token_path);
	ra_result = ra.GetView(token_path, sched_status_view);
	if (ra_result != ResourceAccounterStatusIF::RA_SUCCESS) {
		logger->Fatal("Init: Cannot get a resource state view");
		return ERROR_VIEW;
	}
	logger->Debug("Init: Resources state view token: %d", sched_status_view);

	return result;
}

SchedulerPolicyIF::ExitCode_t
TempuraSchedPol::Schedule(
		System & system,
		RViewToken_t & status_view) {
	SchedulerPolicyIF::ExitCode_t result = SCHED_DONE;

	// Class providing query functions for applications and resources
	sys = &system;

	// Return the new resource status view according to the new resource
	// allocation performed
	status_view = sched_status_view;
	return result;
}

} // namespace plugins

} // namespace bbque
