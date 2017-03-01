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

#include "bbque/tg/task_graph.h"

namespace bbque {


TaskGraph::TaskGraph(uint32_t app_id):
		application_id(app_id) {
	this->is_valid = true;
}

TaskGraph::TaskGraph(
		TaskMap_t const & tasks,
		BufferMap_t const & buffers,
		uint32_t app_id):
	application_id(app_id) {

	bool buff_valid = false;
	for (auto & t_entry: tasks) {
		auto t = t_entry.second;
		buff_valid = AreBuffersValid(t, buffers);
		if (!buff_valid) return;
	}

	this->tasks    = tasks;
	this->buffers  = buffers;
	this->is_valid = true;
}

TaskGraph::TaskGraph(
		TaskMap_t const & tasks,
		BufferMap_t const & buffers,
		EventMap_t const & events,
		uint32_t app_id):
	application_id(app_id) {
	bool buff_valid = false;
	for (auto & t_entry: tasks) {
		auto & t(t_entry.second);
		// buffer validation
		buff_valid = AreBuffersValid(t, buffers);
		if (!buff_valid) return;
		// Add event validation
//		auto evit = events.find(t->GetEvent());
//		if (evit == events.end()) return;
	}

	this->tasks    = tasks;
	this->buffers  = buffers;
	this->events   = events;
	this->is_valid = true;
}


bool TaskGraph::AreBuffersValid(TaskPtr_t task, BufferMap_t const & buffers) {
	for (auto id: task->InputBuffers()) {
		auto b = buffers.find(id);
		if (b == buffers.end()) return false;
	}
	for (auto id: task->OutputBuffers()) {
		auto b = buffers.find(id);
		if (b == buffers.end()) return false;
	}
	return true;
}


/* ========================== Tasks ===================== */
/*
TaskGraph::ExitCode
TaskGraph::AddTask(TaskPtr_t task, BufferPtr_t in_buff, BufferPtr_t out_buff) {
	if (in_buff != nullptr) {
		in_buff->AddOutputTask(task);
		buffers.emplace(in_buff->Id(), in_buff);
	}
	if (out_buff != nullptr) {
		out_buff->AddInputTask(task);
		buffers.emplace(out_buff->Id(), out_buff);
	}

	return AddTask(task);
}

TaskGraph::ExitCode
TaskGraph::AddTask(TaskPtr_t task, int in_buff_id, int out_buff_id) {
	auto in_buff = buffers.find(in_buff_id);
	if (in_buff == buffers.end()) {
		return ERR_INVALID_BUFFER;
	}

	auto out_buff = buffers.find(in_buff_id);
	if (out_buff == buffers.end()) {
		return ERR_INVALID_BUFFER;
	}

	in_buff->second->AddOutputTask(task);
	out_buff->second->AddInputTask(task);
	return AddTask(task);
}
*/


/* ========================== Buffers ===================== */


TaskGraph::ExitCode TaskGraph::SetOutputBuffer(int32_t id) {
	auto outb = buffers.find(id);
	if (outb == buffers.end())
		return ExitCode::ERR_INVALID_BUFFER;
	this->out_buff = outb->second;
	return ExitCode::SUCCESS;
}


} // namespace bbque
