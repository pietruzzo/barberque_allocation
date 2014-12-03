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


#ifndef BBQUE_POWER_MONITOR_H_
#define BBQUE_POWER_MONITOR_H_

#include <cstdint>
#include <map>

#include "bbque/config.h"
#include "bbque/command_manager.h"
#include "bbque/pm/power_manager.h"
#include "bbque/res/resources.h"
#include "bbque/utils/worker.h"
#include "bbque/utils/logging/logger.h"

#define POWER_MONITOR_NAMESPACE "bq.wm"


namespace bu = bbque::utils;
namespace br = bbque::res;

namespace bbque {

class PowerMonitor: public CommandHandler, bu::Worker {

public:

	/**
	 * @enum ExitCode_t
	 *
	 * Class specific return codes
	 */
	enum class ExitCode_t {
		/** Successful call */
		OK = 0,
		/** Not valid resource specified */
		ERR_RSRC_MISSING,
		/** A not specified error code */
		ERR_UNKNOWN
	};

	/** Power Monitor instance */
	static PowerMonitor & GetInstance();

	/**
	 * @brief Destructor
	 */
	virtual ~PowerMonitor();

	/**
	 * @brief Command handlers dispatcher
	 */
	int CommandsCb(int argc, char *argv[]);

private:

	/*
	 * @brief Power manager instance
	 */
	PowerManager & pm;

	/**
	 * @brief Command manager instance
	 */
	CommandManager & cm;

	/**
	 * @brief The logger used by the power manager.
	 */
	std::unique_ptr<bu::Logger> logger;


	/**
	 * @brief Constructor
	 */
	PowerMonitor();

	/**
	 * @brief Periodic task
	 */
	virtual void Task();

};

} // namespace bbque

#endif // BBQUE_POWER_MONITOR_H_

