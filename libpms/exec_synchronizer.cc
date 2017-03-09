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
		std::shared_ptr<TaskGraph> tg) {
	task_graph = tg;
	if (!CheckTaskGraph()) {
		task_graph = nullptr;
		return ExitCode::ERR_TASK_GRAPH_NOT_VALID;
	}

	task_graph->SetApplicationId(GetUid());

	try {
		serial_file_path = BBQUE_TG_FILE_PREFIX + std::string(GetUniqueID_String());
		logger->Info("Task-graph serialization file: %s [uid=%d]", serial_file_path.c_str(),
			GetUniqueID());
	}
	catch (std::exception & ex) {
		logger->Error("Error while creating serialization file name");
		return ExitCode::ERR_TASK_GRAPH_FILE_NAME;
	}

	// Cannot change the TG while still in execution
	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	if (tasks.is_stopped.any()) {
		logger->Error("Tasks execution still in progress...");
		return ExitCode::ERR_TASKS_IN_EXECUTION;
	}

	// Task execution status initialization
	for (auto & t_entry: task_graph->Tasks()) {
		auto & task(t_entry.second);
		tasks.is_stopped.set(task->Id());
		std::shared_ptr<RuntimeInfo> rt_info =
			std::make_shared<RuntimeInfo>(false);
		tasks.runtime.emplace(task->Id(), rt_info);
	}

	// Task synchronization events initialization
	for (auto & ev_entry: task_graph->Events()) {
		auto & event(ev_entry.second);
		std::shared_ptr<EventSync> ev_sync =
			std::make_shared<EventSync>(event->Id());
                events.emplace(event->Id(), ev_sync);
	}

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
		logger->Error("StartTask: unknown task id: %d", task_id);
		return ExitCode::ERR_TASK_ID;
	}

	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	enqueue_task(task);
	tasks.cv.notify_all();
	logger->Info("StartTask: [Task %2d] launched", task_id);

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
			logger->Error("StartTasks: unknown task id: %d", task_id);
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
		logger->Error("StopTask: unknown task id: %d", task_id);
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
			logger->Error("StopTasks: unknown task id: %d", task_id);
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
	return ExitCode::SUCCESS;
}



void ExecutionSynchronizer::NotifyEvent(uint32_t event_id) noexcept {
	auto evit = events.find(event_id);
	if (evit == events.end())
		return;
	auto & event(evit->second);
	std::unique_lock<std::mutex> ev_lock(evit->second->mx);
	event->occurred = true;
	event->cv.notify_all();
	logger->Debug("[Event %2d] notified", event_id);
}



void ExecutionSynchronizer::WaitForResourceAllocation() noexcept {
	std::unique_lock<std::mutex> rtrm_lock(rtrm.mx);
	while (!rtrm.scheduled)
		rtrm.cv.wait(rtrm_lock);
}


// ---------------- Protected/private ---------------------------------------//


void ExecutionSynchronizer::NotifyResourceAllocation() noexcept {
	std::unique_lock<std::mutex> rtrm_lock(rtrm.mx);
	rtrm.scheduled = true;
	rtrm.cv.notify_all();
}

void ExecutionSynchronizer::TaskProfiler(uint32_t task_id) noexcept {

	auto task = task_graph->GetTask(task_id);
	if (task == nullptr) {
		logger->Crit("[Task %2d] missing task descriptor", task_id);
		return;
	}

	auto out_buffers = task->OutputBuffers();
	if (out_buffers.empty()) {
		logger->Error("[Task %2d] missing output buffers", task_id);
		return;
	}

	// Pick the first output buffer of the task
	auto buffer = task_graph->GetBuffer(out_buffers.front());
	if (buffer == nullptr) {
		logger->Error("[Task %2d] missing output buffer descriptor", task_id);
		return;
	}

	auto evit = events.find(buffer->Event());
	if (evit == events.end()) {
		logger->Error("[Task %2d] missing event(%d) associated to buffer %d",
			task_id, buffer->Event(), buffer->Id());
		return;
	}

	auto & event(evit->second);
	auto & prof_data = tasks.runtime[task_id]->profile;
	prof_data.timer.start();
	double t_curr, t_start, t_finish;

	// Synchronize the profiling timing according to the events (write)
	// affecting the output buffer of the task
	logger->Info("[Task %2d] profiling started", task_id);
	while (tasks.runtime[task_id]->is_running) {
		std::unique_lock<std::mutex> ev_lock(event->mx);
		t_start  = prof_data.timer.getElapsedTimeUs();
		event->cv.wait(ev_lock);
		t_finish = prof_data.timer.getElapsedTimeUs();
		t_curr = t_finish - t_start;
		prof_data.acc(t_curr);
		logger->Debug("[Task %d] timing current = %.2f us", task_id, t_curr);
	}
	prof_data.timer.stop();
	logger->Info("[Task %2d] profiling stopped", task_id);
}

// ---------------- BbqueEXC overloading ------------------------------------//

RTLIB_ExitCode_t ExecutionSynchronizer::onSetup() {
	if (!CheckTaskGraph())
		return RTLIB_ERROR;

	// Synchronization event for onRun() return
	auto outb = task_graph->OutputBuffer();
	if (outb == nullptr) {
		logger->Error("onSetup: Task-graph output buffer missing");
		return RTLIB_ERROR;
	}

	auto ev = task_graph->GetEvent(outb->Event());
	if (ev == nullptr) {
		logger->Error("onSetup: Task-graph synchronization event missing");
		return RTLIB_ERROR;
	}

	on_run_sync = events[ev->Id()];
	logger->Info("onSetup: Task-graph synchronization event_id = %d", on_run_sync->id);

	// Send the task graph
	SendTaskGraphToRM();
	logger->Info("onSetup: [Application %s] starting...", app_name.c_str());

	return RTLIB_OK;
}


RTLIB_ExitCode_t ExecutionSynchronizer::onConfigure(int8_t awm_id) {
	(void) awm_id;

	// Task graph here should has been filled by the RTRM policy
	RecvTaskGraphFromRM();
	logger->Info("onConfigure: Resource allocation performed");
	NotifyResourceAllocation();

	// Wait for tasks to start
	std::unique_lock<std::mutex> tasks_lock(tasks.mx);
	logger->Info("onConfigure: Waiting for tasks to start...");
	while (tasks.is_stopped.any()) {
		logger->Debug("Tasks stopped: %s", tasks.is_stopped.to_string().c_str());
		tasks.cv.wait(tasks_lock);
	}

	logger->Info("onConfigure: Tasks queue length: %d", tasks.start_queue.size());
	while (!tasks.start_queue.empty()) {
		auto task_id = tasks.start_queue.front();
		tasks.runtime[task_id]->monitor_thr = std::move(
			std::thread(&ExecutionSynchronizer::TaskProfiler, this, task_id));
		tasks.start_queue.pop();
		logger->Info("onConfigure: [Task %2d] started on processor %d", task_id,
				task_graph->GetTask(task_id)->GetMappedProcessor());
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t ExecutionSynchronizer::onRun() {
	{
		std::unique_lock<std::mutex> tasks_lock(tasks.mx);
		if (tasks.is_stopped.count() == task_graph->TaskCount()) {
			logger->Info("onRun: Termination...");
			return RTLIB_EXC_WORKLOAD_NONE;
		}
	}

	// Wait for synchronization event: task-graph output
	std::unique_lock<std::mutex> run_lock(on_run_sync->mx);
	while (!on_run_sync->occurred) {
		on_run_sync->cv.wait(run_lock);
		logger->Info("onRun: Cycle %02d sync_event = %d",
			Cycles(), on_run_sync->id);
	}
	on_run_sync->occurred = false;

	return RTLIB_OK;
}


RTLIB_ExitCode_t ExecutionSynchronizer::onMonitor() {

	for (auto & rt_entry: tasks.runtime) {
		auto & prof_data(rt_entry.second->profile);

		uint16_t task_tput =
			static_cast<uint16_t>(1e6 / (mean(prof_data.acc)));
		logger->Info("onMonitor: [Task %2d] timing mean=%.2fus tput=%.2f",
			rt_entry.first, mean(prof_data.acc), task_tput);
		task_graph->GetTask(rt_entry.first)->SetProfiling(task_tput*100, 0);
	}

	logger->Info("onMonitor: Task-graph throughput: CPS:=%.2f", GetCPS());
	task_graph->SetProfiling(GetCPS()*100, 0);
	SendTaskGraphToRM();

	return RTLIB_OK;
}

RTLIB_ExitCode_t ExecutionSynchronizer::onRelease() {

	for (auto & ev_entry: events)
		NotifyEvent(ev_entry.first);

	for (auto & rt_entry: tasks.runtime) {
		auto & monitor(rt_entry.second->monitor_thr);
		monitor.join();
		logger->Info("onRelease: Monitoring thread joined");
	}

	return RTLIB_OK;
}

// ---------------------------------------------------------------------------//


} // namespace bbque
