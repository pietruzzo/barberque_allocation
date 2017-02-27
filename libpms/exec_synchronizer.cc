/*
 * Copyright (C) 2017  Politecnico di Milano
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

#include <fstream>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include "pmsl/exec_synchronizer.h"


namespace bbque {

ExecutionSynchronizer::ExecutionSynchronizer(
		std::string const & name,
		std::string const & recipe,
		RTLIB_Services_t * rtlib):
	BbqueEXC(name, recipe, rtlib),
	app_name(name) {
}


ExecutionSynchronizer::ExecutionSynchronizer(
		std::string const & name,
		std::string const & recipe,
		RTLIB_Services_t * rtlib,
		std::shared_ptr<TaskGraph> tg):
	BbqueEXC(name, recipe, rtlib),
	app_name(name),
	task_graph(tg) {
}


ExecutionSynchronizer::ExitCode ExecutionSynchronizer::SetTaskGraph(
		std::shared_ptr<TaskGraph> tg) noexcept {
	task_graph = tg;
	if (!CheckTaskGraph()) {
		task_graph = nullptr;
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;
	}

	serial_file_path = BBQUE_TG_SERIAL_FILE
		+ std::to_string(tg->GetApplicationId())
		+ exc_name;

	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	if (tasks.is_stopped.any()) {
		// Cannot change the TG while still in execution
		return ExitCode::ERR_TASKS_IN_EXECUTION;
	}

	// Task execution status initialization
	for (auto & t_entry: task_graph->Tasks()) {
		auto & task(t_entry.second);
		tasks.is_stopped.set(task->Id());
	}

	// Task synchronization events initialization
	for (auto & ev_entry: task_graph->Events()) {
		auto & event(ev_entry.second);
		std::shared_ptr<EventSync> ev_sync =
			std::make_shared<EventSync>(event->Id());
                events.emplace(event->Id(), ev_sync);
	}

	logger->Error("Task status: %s", tasks.is_stopped.to_string().c_str());
	tasks.cv.notify_all();
	return ExitCode::SUCCESS;
}


bool ExecutionSynchronizer::CheckTaskGraph() noexcept {
	if (task_graph == nullptr) {
		logger->Error("Task graph missing");
		return false;
	}

	if (!task_graph->IsValid()) {
		logger->Error("Task graph not valid");
		return false;
	}
	return true;
}


void ExecutionSynchronizer::SendTaskGraphToRM() {
	std::ofstream ofs(serial_file_path);
	boost::archive::text_oarchive oa(ofs);
	oa << *(this->task_graph);
	logger->Info("Task-graph sent for resource allocation");
}

void ExecutionSynchronizer::RecvTaskGraphFromRM() {
	std::ifstream ifs(serial_file_path);
	boost::archive::text_iarchive ia(ifs);
	ia >> *(this->task_graph);
	logger->Info("Task-graph restored after resource allocation");
}


ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StartTask(
		uint32_t task_id) noexcept {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	auto task = task_graph->GetTask(task_id);
	if (task == nullptr) {
		logger->Error("Unknown task id: %d", task_id);
		return ExitCode::ERR_TASK_ID;
	}

	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	enqueue_task(task);
	tasks.cv.notify_all();
	return ExitCode::SUCCESS;
}

ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StartTasks(
		std::list<uint32_t> tasks_ids) noexcept {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	for (auto task_id: tasks_ids) {
		auto task = task_graph->GetTask(task_id);
		if (task == nullptr) {
			logger->Error("Unknown task id: %d", task_id);
			return ExitCode::ERR_TASK_ID;
		}
		enqueue_task(task);
	}
	tasks.cv.notify_all();
	return ExitCode::SUCCESS;
}

ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StartTasksAll() noexcept {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	for (auto t_entry: task_graph->Tasks()) {
		auto & task = t_entry.second;
		enqueue_task(task);
	}
	tasks.cv.notify_all();
	return ExitCode::SUCCESS;
}


ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StopTask(
		uint32_t task_id) noexcept {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	auto task = task_graph->GetTask(task_id);
	if (task == nullptr) {
		logger->Error("Unknown task id: %d", task_id);
			return ExitCode::ERR_TASK_ID;
	}

	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	dequeue_task(task);
	tasks.cv.notify_all();
	return ExitCode::SUCCESS;
}

ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StopTasks(
		std::list<uint32_t> tasks_ids) noexcept {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	for (auto task_id: tasks_ids) {
		auto task = task_graph->GetTask(task_id);
		if (task == nullptr) {
			logger->Error("Unknown task id: %d", task_id);
			return ExitCode::ERR_TASK_ID;
		}
		dequeue_task(task);
	}
	tasks.cv.notify_all();
	return ExitCode::SUCCESS;
}

ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StopTasksAll() noexcept {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	for (auto & t_entry: task_graph->Tasks()) {
		auto & task(t_entry.second);
		dequeue_task(task);
	}
	tasks.cv.notify_all();
}



void ExecutionSynchronizer::NotifyEvent(uint32_t event_id) noexcept {
	auto evit = events.find(event_id);
	if (evit == events.end())
		return;
	std::unique_lock<std::mutex> ev_lock(evit->second->mx);
	evit->second->occurred = true;
	evit->second->cv.notify_all();
	logger->Info("[Event %2d] notified", event_id);
}



void ExecutionSynchronizer::WaitForResourceAllocation() noexcept {
	std::unique_lock<std::mutex> rtrm_ul(rtrm.mx);
	while (!rtrm.scheduled)
		rtrm.cv.wait(rtrm_ul);
}


// ---------------- Protected/private ---------------------------------------//


void ExecutionSynchronizer::NotifyResourceAllocation() noexcept {
	std::unique_lock<std::mutex> rtrm_ul(rtrm.mx);
	rtrm.scheduled = true;
	rtrm.cv.notify_all();
}

void ExecutionSynchronizer::StartTaskControl(uint32_t task_id) noexcept {

}


		return;
}

// ---------------- BbqueEXC overloading ------------------------------------//

RTLIB_ExitCode_t ExecutionSynchronizer::onSetup() {
	if (!CheckTaskGraph())
		return RTLIB_ERROR;

	// Synchronization event for onRun() return
	auto outb = task_graph->OutputBuffer();
	if (outb == nullptr) {
		logger->Error("Task-graph output buffer missing");
		return RTLIB_ERROR;
	}

	auto ev = task_graph->GetEvent(outb->Event());
	if (ev == nullptr) {
		logger->Error("Task-graph synchronization event missing");
		return RTLIB_ERROR;
	}

	on_run_sync = events[ev->Id()];
	logger->Info("Task-graph synchronization event_id = %d", on_run_sync->id);

	// Send the task graph
	SendTaskGraphToRM();
	logger->Info("[Application %s] starting...", app_name.c_str());

	return RTLIB_OK;
}


RTLIB_ExitCode_t ExecutionSynchronizer::onConfigure(int8_t awm_id) {
	(void) awm_id;

	// Task graph here should has been filled by the RTRM policy
	RecvTaskGraphFromRM();
	logger->Info("Resource allocation performed");
	NotifyResourceAllocation();

	// Wait for tasks to start
	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	logger->Info("Tasks status: %s", tasks.is_stopped.to_string().c_str());
	while (tasks.is_stopped.any()) {
		logger->Warn("Waiting for tasks to start...");
		tasks.cv.wait(tasks_lock);
	}

	logger->Info("Tasks queue length: %d", tasks.start_queue.size());
	while (!tasks.start_queue.empty()) {
		auto task_id = tasks.start_queue.front();
		StartTaskControl(task_id);
		tasks.start_queue.pop();
		tasks.is_stopped.reset(task_id);
		logger->Info("[Task %2d] started on processor %d", task_id,
				task_graph->GetTask(task_id)->GetMappedProcessor());
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t ExecutionSynchronizer::onRun() {
	{
		std::unique_lock<std::mutex> tasks_lock(tasks.mx);
		if (tasks.is_stopped.count() == task_graph->TaskCount()) {
			logger->Info("Termination...");
			return RTLIB_EXC_WORKLOAD_NONE;
		}
	}

	// Wait for synchronization event: task-graph output
	std::unique_lock<std::mutex> run_lock(on_run_sync->mx);
	while (!on_run_sync->occurred) {
		on_run_sync->cv.wait(run_lock);
		logger->Info("<onRun> cycle = %d sync_event = %d",
			Cycles(), on_run_sync->id);
	}
	on_run_sync->occurred = false;

	return RTLIB_OK;
}


RTLIB_ExitCode_t ExecutionSynchronizer::onMonitor() {

	return RTLIB_OK;
}

RTLIB_ExitCode_t ExecutionSynchronizer::onRelease() {

	return RTLIB_OK;

}

// ---------------------------------------------------------------------------//


} // namespace bbque
