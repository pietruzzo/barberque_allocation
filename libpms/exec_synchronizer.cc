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
	if (tasks_start_status.test(t->Id())) { \
		tasks_start_queue.push(t->Id()); \
		tasks_start_status.reset(t->Id()); \
	}

#define dequeue_task(t) \
	if (!tasks_start_status.test(t->Id())) { \
		tasks_start_status.set(t->Id()); \
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


ExecutionSynchronizer::ExitCode ExecutionSynchronizer::SetTaskGraph(std::shared_ptr<TaskGraph> tg) {
	task_graph = tg;
	if (!CheckTaskGraph()) {
		task_graph = nullptr;
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;
	}

	file_path  = BBQUE_TG_SERIAL_FILE
		+ std::to_string(tg->GetApplicationId())
		+ exc_name;

	std::unique_lock<std::mutex> tasks_lock(tasks_mx);
	if (tasks_start_status.any()) {
		// Cannot change the TG while still in execution
		return ExitCode::ERR_TASKS_IN_EXECUTION;
	}

	for (auto t_entry: task_graph->Tasks()) {
		auto & task = t_entry.second;
		tasks_start_status.set(task->Id());
	}

	std::cerr << "Task status: " << tasks_start_status.to_string() << std::endl;
	tasks_cv.notify_all();
	return ExitCode::SUCCESS;
}


bool ExecutionSynchronizer::CheckTaskGraph() {
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
	std::ofstream ofs(file_path);
	boost::archive::text_oarchive oa(ofs);
	oa << *(this->task_graph);
}

void ExecutionSynchronizer::RecvTaskGraphFromRM() {
	std::ifstream ifs(file_path);
	boost::archive::text_iarchive ia(ifs);
	ia >> *(this->task_graph);
}


ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StartTask(uint32_t task_id) {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	auto task = task_graph->GetTask(task_id);
	if (task == nullptr) {
		logger->Error("Unknown task id: %d", task_id);
		return ExitCode::ERR_TASK_ID;
	}

	std::unique_lock<std::mutex> tasks_lock(tasks_mx);
	enqueue_task(task);
	tasks_cv.notify_all();
	return ExitCode::SUCCESS;
}

ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StartTasks(std::list<uint32_t> tasks_ids) {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	std::unique_lock<std::mutex> tasks_lock(tasks_mx);
	for (auto task_id: tasks_ids) {
		auto task = task_graph->GetTask(task_id);
		if (task == nullptr) {
			logger->Error("Unknown task id: %d", task_id);
			return ExitCode::ERR_TASK_ID;
		}
		enqueue_task(task);
	}
	tasks_cv.notify_all();
	return ExitCode::SUCCESS;
}

ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StartTasksAll() {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	std::unique_lock<std::mutex> tasks_lock(tasks_mx);
	for (auto t_entry: task_graph->Tasks()) {
		auto & task = t_entry.second;
		enqueue_task(task);
	}
	tasks_cv.notify_all();
	return ExitCode::SUCCESS;

}


ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StopTask(uint32_t task_id) {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	auto task = task_graph->GetTask(task_id);
	if (task == nullptr) {
		logger->Error("Unknown task id: %d", task_id);
			return ExitCode::ERR_TASK_ID;
	}

	std::unique_lock<std::mutex> tasks_lock(tasks_mx);
	dequeue_task(task);
	tasks_cv.notify_all();
	return ExitCode::SUCCESS;

}

ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StopTasks(std::list<uint32_t> tasks_ids) {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	std::unique_lock<std::mutex> tasks_lock(tasks_mx);
	for (auto task_id: tasks_ids) {
		auto task = task_graph->GetTask(task_id);
		if (task == nullptr) {
			logger->Error("Unknown task id: %d", task_id);
			return ExitCode::ERR_TASK_ID;
		}
		dequeue_task(task);
	}
	tasks_cv.notify_all();
	return ExitCode::SUCCESS;

}

ExecutionSynchronizer::ExitCode ExecutionSynchronizer::StopTasksAll() {
	if (!CheckTaskGraph())
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;

	std::unique_lock<std::mutex> tasks_lock(tasks_mx);
	for (auto t_entry: task_graph->Tasks()) {
		auto & task = t_entry.second;
		dequeue_task(task);
	}
	tasks_cv.notify_all();
}

void ExecutionSynchronizer::WaitForResourceAllocation() {
	std::unique_lock<std::mutex> rtrm_ul(rtrm_mx);
	while (!resources_assigned)
		rtrm_cv.wait(rtrm_ul);
}

void ExecutionSynchronizer::NotifyResourceAllocation() {
	std::unique_lock<std::mutex> rtrm_ul(rtrm_mx);
	resources_assigned = true;
	rtrm_cv.notify_all();
}

void ExecutionSynchronizer::StartTaskControl(uint32_t task_id) {

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
	std::unique_lock<std::mutex> tasks_lock(tasks_mx);
	while (tasks_start_status.any()) {
		tasks_cv.wait(tasks_lock);
		while (!tasks_start_queue.empty()) {
			auto task_id = tasks_start_queue.front();
			StartTaskControl(task_id);
			tasks_start_queue.pop();
			tasks_start_status.reset(task_id);
		}
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t ExecutionSynchronizer::onRun() {

	return RTLIB_OK;
}


RTLIB_ExitCode_t ExecutionSynchronizer::onMonitor() {

	return RTLIB_OK;
}

RTLIB_ExitCode_t ExecutionSynchronizer::onRelease() {

	return RTLIB_OK;

}


}
