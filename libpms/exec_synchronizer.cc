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

#define enqueue_task(t) \
	if (tasks.start_status.test(t->Id())) { \
		tasks.start_queue.push(t->Id()); \
		tasks.start_status.reset(t->Id()); \
	}

#define dequeue_task(t) \
	if (!tasks.start_status.test(t->Id())) { \
		tasks.start_status.set(t->Id()); \
	}

namespace bbque {

ExecutionSynchronizer::ExecutionSynchronizer(
		std::string const & name,
		std::string const & recipe,
		RTLIB_Services_t * rtlib):
	BbqueEXC(name, recipe, rtlib) {
}


ExecutionSynchronizer::ExecutionSynchronizer(
		std::string const & name,
		std::string const & recipe,
		RTLIB_Services_t * rtlib,
		std::shared_ptr<TaskGraph> tg):
	BbqueEXC(name, recipe, rtlib),
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
	if (tasks.start_status.any()) {
		// Cannot change the TG while still in execution
		return ExitCode::ERR_TASKS_IN_EXECUTION;
	}

	for (auto t_entry: task_graph->Tasks()) {
		auto & task = t_entry.second;
		tasks.start_status.set(task->Id());
	}

	logger->Error("Task status: %s", tasks.start_status.to_string().c_str());
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
}

void ExecutionSynchronizer::RecvTaskGraphFromRM() {
	std::ifstream ifs(serial_file_path);
	boost::archive::text_iarchive ia(ifs);
	ia >> *(this->task_graph);
}


ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StartTask(uint32_t task_id) noexcept {
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


ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StopTask(uint32_t task_id) noexcept {
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
	for (auto t_entry: task_graph->Tasks()) {
		auto & task = t_entry.second;
		dequeue_task(task);
	}
	tasks.cv.notify_all();
}

void ExecutionSynchronizer::WaitForResourceAllocation() noexcept {
	std::unique_lock<std::mutex> rtrm_ul(rtrm.mx);
	while (!rtrm.scheduled)
		rtrm.cv.wait(rtrm_ul);
}

void ExecutionSynchronizer::NotifyResourceAllocation() noexcept {
	std::unique_lock<std::mutex> rtrm_ul(rtrm.mx);
	rtrm.scheduled = true;
	rtrm.cv.notify_all();
}

void ExecutionSynchronizer::StartTaskControl(uint32_t task_id) noexcept {

}


RTLIB_ExitCode_t ExecutionSynchronizer::onSetup() {
	if (!CheckTaskGraph())
		return RTLIB_ERROR;

	SendTaskGraphToRM();
	logger->Info("Application [%s] starting...", exc_name);

	return RTLIB_OK;
}



RTLIB_ExitCode_t ExecutionSynchronizer::onConfigure(int8_t awm_id) {
	(void) awm_id;

	// Task graph here should has been filled by the RTRM policy
	RecvTaskGraphFromRM();
	NotifyResourceAllocation();

	// Update the tasks status
	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	while (tasks.start_status.any()) {
		tasks.cv.wait(tasks_lock);
		while (!tasks.start_queue.empty()) {
			auto task_id = tasks.start_queue.front();
			StartTaskControl(task_id);
			tasks.start_queue.pop();
			tasks.start_status.reset(task_id);
			logger->Info("Task [%d] started", task_id);
		}
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t ExecutionSynchronizer::onRun() {
	{
		std::unique_lock<std::mutex> tasks_lock(tasks.mx);
		if (tasks.start_status.count() == task_graph->TaskCount()) {
			logger->Notice("Termination...");
			return RTLIB_EXC_WORKLOAD_NONE;
		}
	}

	return RTLIB_OK;
}


RTLIB_ExitCode_t ExecutionSynchronizer::onMonitor() {

	return RTLIB_OK;
}

RTLIB_ExitCode_t ExecutionSynchronizer::onRelease() {

	return RTLIB_OK;

}


}
