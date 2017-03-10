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

#include "test_schedpol.h"

#include <cstdlib>
#include <cstdint>
#include <iostream>

#include "bbque/modules_factory.h"
#include "bbque/utils/logging/logger.h"

#include "bbque/app/working_mode.h"
#include "bbque/res/binder.h"
#include "bbque/res/resource_path.h"
#include "tg/task_graph.h"

#define MODULE_CONFIG SCHEDULER_POLICY_CONFIG "." SCHEDULER_POLICY_NAME

namespace bu = bbque::utils;
namespace po = boost::program_options;

namespace bbque { namespace plugins {

// :::::::::::::::::::::: Static plugin interface ::::::::::::::::::::::::::::

void * TestSchedPol::Create(PF_ObjectParams *) {
	return new TestSchedPol();
}

int32_t TestSchedPol::Destroy(void * plugin) {
	if (!plugin)
		return -1;
	delete (TestSchedPol *)plugin;
	return 0;
}

// ::::::::::::::::::::: Scheduler policy module interface :::::::::::::::::::

char const * TestSchedPol::Name() {
	return SCHEDULER_POLICY_NAME;
}

TestSchedPol::TestSchedPol():
		cm(ConfigurationManager::GetInstance()),
		ra(ResourceAccounter::GetInstance()) {
	logger = bu::Logger::GetLogger("bq.sp.test");
	assert(logger);
	if (logger)
		logger->Info("test: Built a new dynamic object[%p]", this);
	else
		fprintf(stderr,
			FI("test: Built new dynamic object [%p]\n"), (void *)this);
}


TestSchedPol::~TestSchedPol() {

}


SchedulerPolicyIF::ExitCode_t TestSchedPol::Init() {
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

	logger->Debug("Init: loading the applications task graphs");
	fut_tg = std::async(std::launch::async, &System::LoadTaskGraphs, sys);

	return SCHED_OK;
}


SchedulerPolicyIF::ExitCode_t
TestSchedPol::Schedule(
		System & system,
		RViewToken_t & status_view) {
	SchedulerPolicyIF::ExitCode_t result = SCHED_DONE;

	// Class providing query functions for applications and resources
	sys = &system;
	Init();

	/** INSERT YOUR CODE HERE **/
	bbque::app::AppCPtr_t papp;
	AppsUidMapIt app_it;

	fut_tg.get();

	// Ready applications
	papp = sys->GetFirstReady(app_it);
	for (; papp; papp = sys->GetNextReady(app_it)) {
		AssignWorkingMode(papp);
	}

	// Running applications
	papp = sys->GetFirstRunning(app_it);
	for (; papp; papp = sys->GetNextRunning(app_it)) {
		papp->CurrentAWM()->ClearResourceRequests();
		AssignWorkingMode(papp);
	}

	// Return the new resource status view according to the new resource
	// allocation performed
	status_view = sched_status_view;
	return result;
}

SchedulerPolicyIF::ExitCode_t
TestSchedPol::AssignWorkingMode(bbque::app::AppCPtr_t papp) {

	// Build a new working mode featuring assigned resources
	ba::AwmPtr_t pawm = papp->CurrentAWM();
	if (pawm == nullptr) {
		pawm = std::make_shared<ba::WorkingMode>(
				papp->WorkingModes().size(),"Run-time", 1, papp);
	}

	// Resource request addition
	pawm->AddResourceRequest("sys0.cpu.pe", 200, br::ResourceAssignment::Policy::BALANCED);

	// The ResourceBitset object is used for the processing elements binding
	// (CPU core mapping)
	br::ResourceBitset pes;
	pes.Set(0);

	int32_t ref_num = -1;

	// CPU-level binding: the processing elements are in the scope of CPU '1'
	ref_num = pawm->BindResource(br::ResourceType::CPU, R_ID_ANY, 1, ref_num);
	auto resource_path = ra.GetPath("sys0.cpu1.pe");

	ref_num = pawm->BindResource(resource_path, pes, ref_num);
	logger->Info("Reference number for binding: %d", ref_num);

	pes.Set(1);
	ref_num = pawm->BindResource(resource_path, pes, ref_num);
	logger->Info("Reference number for binding: %d", ref_num);

	papp->ScheduleRequest(pawm, sched_status_view, ref_num);

	// Task level mapping
	MapTaskGraph(papp);

	/*
	// Enqueue scheduling entity
	SchedEntityPtr_t psched = std::make_shared<SchedEntity_t>(
			papp, pawm, R_ID_ANY, 0);
	entities.push_back(psched);
	*/

	return SCHED_OK;
}


void TestSchedPol::MapTaskGraph(bbque::app::AppCPtr_t papp) {
	auto task_graph = papp->GetTaskGraph();
	if (task_graph == nullptr) {
		logger->Warn("[%s] No task-graph to map", papp->StrId());
		return;
	}

	uint16_t throughput;
	uint32_t c_time;
	int unit_id = 3; // An arbitrary device id number

	for (auto t_entry: task_graph->Tasks()) {
		unit_id++;
		auto & task(t_entry.second);
		task->SetMappedProcessor(unit_id);
		task->GetProfiling(throughput, c_time);
		logger->Info("[%s] <T %d> throughput: %d  ctime: %d",
			papp->StrId(), t_entry.first, throughput, c_time);
	}

	task_graph->GetProfiling(throughput, c_time);
	logger->Info("[%s] Task-graph throughput: %d  ctime: %d",
		papp->StrId(), throughput, c_time);

	papp->SetTaskGraph(task_graph);
	logger->Info("[%s] Task-graph updated", papp->StrId());
}


} // namespace plugins

} // namespace bbque
