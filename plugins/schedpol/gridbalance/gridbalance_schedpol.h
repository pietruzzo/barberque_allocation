/*
 * Copyright (C) 2014  Politecnico di Milano
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

#ifndef BBQUE_GRIDBALANCE_SCHEDPOL_H_
#define BBQUE_GRIDBALANCE_SCHEDPOL_H_

#include <cstdint>
#include <list>
#include <memory>

#include "bbque/configuration_manager.h"
#include "bbque/plugins/plugin.h"
#include "bbque/plugins/scheduler_policy.h"
#include "bbque/scheduler_manager.h"

#define SCHEDULER_POLICY_NAME "gridbalance"

#define MODULE_NAMESPACE SCHEDULER_POLICY_NAMESPACE "." SCHEDULER_POLICY_NAME

using bbque::res::RViewToken_t;
using bbque::utils::MetricsCollector;
using bbque::utils::Timer;

// These are the parameters received by the PluginManager on create calls
struct PF_ObjectParams;

namespace bbque { namespace plugins {

class LoggerIF;

/**
 * @class GridBalanceSchedPol
 *
 * GridBalance scheduler policy registered as a dynamic C++ plugin.
 */
class GridBalanceSchedPol: public SchedulerPolicyIF {

public:

	// :::::::::::::::::::::: Static plugin interface :::::::::::::::::::::::::

	/**
	 * @brief Create the gridbalance plugin
	 */
	static void * Create(PF_ObjectParams *);

	/**
	 * @brief Destroy the gridbalance plugin
	 */
	static int32_t Destroy(void *);


	// :::::::::::::::::: Scheduler policy module interface :::::::::::::::::::

	/**
	 * @brief Destructor
	 */
	virtual ~GridBalanceSchedPol();

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

	ConfigurationManager & cm;

	ResourceAccounter & ra;

	std::unique_ptr<bu::Logger> logger;





	GridBalanceSchedPol();

	// ----- Initialization stuff ---- //

	ExitCode_t Init();


};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_GRIDBALANCE_SCHEDPOL_H_
