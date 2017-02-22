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

#ifndef BBQUE_EXC_SYNCHRONIZER_H_
#define BBQUE_EXC_SYNCHRONIZER_H_

#include <bitset>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>

#include "bbque/bbque_exc.h"
#include "bbque/tg/task_graph.h"

#define BBQUE_TG_SERIAL_FILE "/tmp/tg_"


namespace bbque {

class ExecutionSynchronizer: public bbque::rtlib::BbqueEXC {

public:

	enum class EventType {
	    NONE,
	    TASK_START,
	    TASK_COMPLETE,
	    BUFFER_READ,
	    BUFFER_WRITE
	};

	enum class ExitCode {
		SUCCESS,
		ERR_TASK_ID,
		ERR_TASK_GRAPH_NOT_VALID,
		ERR_TASKS_IN_EXECUTION
	};


	struct Event {
		std::mutex mx;
		std::condition_variable cv;
		EventType event;
	};

	ExecutionSynchronizer(
		std::string const & name,
		std::string const & recipe,
		RTLIB_Services_t * rtlib);

	ExecutionSynchronizer(
		std::string const & name,
		std::string const & recipe,
		RTLIB_Services_t * rtlib,
		std::shared_ptr<TaskGraph> tg);

	virtual ~ExecutionSynchronizer() {}


	ExitCode SetTaskGraph(std::shared_ptr<TaskGraph> tg);

	inline std::shared_ptr<TaskGraph> GetTaskGraph() {
		return task_graph;
	}

	ExitCode StartTask(uint32_t task_id);

	ExitCode StartTasks(std::list<uint32_t> tasks_id);

	ExitCode StartTasksAll();

	ExitCode StopTask(uint32_t task_id);

	ExitCode StopTasks(std::list<uint32_t> tasks_id);

	ExitCode StopTasksAll();


	void WaitForResourceAllocation();

	inline bool IsResourceAllocationReady() {
		std::unique_lock<std::mutex> rtrm_ul(rtrm_mx);
		return resources_assigned;
	}

protected:

	std::string file_path;

	std::shared_ptr<TaskGraph> task_graph;

	std::vector<Event> tasks_events;

	std::bitset<BBQUE_TASKS_MAX_NUM> tasks_start_status;

	std::queue<uint32_t> tasks_start_queue;

	std::mutex tasks_mx;

	std::condition_variable tasks_cv;

	bool resources_assigned;

	std::mutex rtrm_mx;

	std::condition_variable rtrm_cv;


	RTLIB_ExitCode_t onSetup();

	RTLIB_ExitCode_t onConfigure(int8_t awm_id);

	RTLIB_ExitCode_t onRun();

	RTLIB_ExitCode_t onMonitor();

	RTLIB_ExitCode_t onRelease();


	bool CheckTaskGraph();

	void SendTaskGraphToRM();

	void RecvTaskGraphFromRM();

	void NotifyResourceAllocation();

	void StartTaskControl(uint32_t task_id);

};


}


#endif // BBQUE_EXC_SYNCHRONIZER_H_
