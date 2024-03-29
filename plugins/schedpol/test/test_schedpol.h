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

#ifndef BBQUE_TEST_SCHEDPOL_H_
#define BBQUE_TEST_SCHEDPOL_H_

#include <cstdint>
#include <list>
#include <memory>
#include <future>

#include "bbque/config.h"
#include "bbque/configuration_manager.h"
#include "bbque/plugins/plugin.h"
#include "bbque/plugins/scheduler_policy.h"
#include "bbque/process_manager.h"
#include "bbque/scheduler_manager.h"

#define SCHEDULER_POLICY_NAME "test"

#define MODULE_NAMESPACE SCHEDULER_POLICY_NAMESPACE "." SCHEDULER_POLICY_NAME

using bbque::res::RViewToken_t;
using bbque::utils::MetricsCollector;
using bbque::utils::Timer;

// These are the parameters received by the PluginManager on create calls
struct PF_ObjectParams;

namespace bbque { namespace plugins {

class LoggerIF;

/**
 * @class TestSchedPol
 *
 * Test scheduler policy registered as a dynamic C++ plugin.
 */
class TestSchedPol: public SchedulerPolicyIF {

public:

	// :::::::::::::::::::::: Static plugin interface :::::::::::::::::::::::::

	/**
	 * @brief Create the test plugin
	 */
	static void * Create(PF_ObjectParams *);

	/**
	 * @brief Destroy the test plugin
	 */
	static int32_t Destroy(void *);


	// :::::::::::::::::: Scheduler policy module interface :::::::::::::::::::

	/**
	 * @brief Destructor
	 */
	virtual ~TestSchedPol();

	/**
	 * @brief Return the name of the policy plugin
	 */
	char const * Name();


	/**
	 * @brief The member function called by the SchedulerManager to perform a
	 * new scheduling / resource allocation
	 */
	ExitCode_t Schedule(System & system, RViewToken_t & status_view);

private:

	/** Configuration manager instance */
	ConfigurationManager & cm;

	/** Resource accounter instance */
	ResourceAccounter & ra;

	/** System logger instance */
	std::unique_ptr<bu::Logger> logger;

	std::future<void> fut_tg;

	std::set<BBQUE_RID_TYPE> pe_ids;

	uint32_t nr_apps;

	/**
	 * @brief Constructor
	 *
	 * Plugins objects could be build only by using the "create" method.
	 * Usually the PluginManager acts as object
	 */
	TestSchedPol();

	/**
	 * @brief Optional initialization member function
	 */

	ExitCode_t Init();

	ExitCode_t ScheduleApplications();

#ifdef CONFIG_BBQUE_LINUX_PROC_MANAGER

	ExitCode_t ScheduleProcesses();

	ExitCode_t AssignWorkingMode(ProcPtr_t proc);

#endif // CONFIG_BBQUE_LINUX_PROC_MANAGER

	ExitCode_t AssignWorkingMode(bbque::app::AppCPtr_t papp);

	int32_t DoCPUBinding(bbque::app::AwmPtr_t pawm, BBQUE_RID_TYPE id);

#ifdef CONFIG_BBQUE_TG_PROG_MODEL

	void MapTaskGraph(bbque::app::AppCPtr_t papp);

#endif // CONFIG_BBQUE_TG_PROG_MODEL

};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_TEST_SCHEDPOL_H_
