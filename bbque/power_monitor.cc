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

#include "bbque/power_monitor.h"

#include "bbque/res/resource_path.h"

#define MODULE_NAMESPACE POWER_MONITOR_NAMESPACE

namespace bbque {


PowerMonitor & PowerMonitor::GetInstance() {
	static PowerMonitor instance;
	return instance;
}

PowerMonitor::PowerMonitor():
		Worker(),
		pm(PowerManager::GetInstance()),
		cm(CommandManager::GetInstance()) {
	// Get a logger module
	logger = bu::Logger::GetLogger(POWER_MONITOR_NAMESPACE);
	assert(logger);

	//---------- Setup Worker
	Worker::Setup(BBQUE_MODULE_NAME("wm"), POWER_MONITOR_NAMESPACE);
	Worker::Start();
}
}

PowerMonitor::~PowerMonitor() {

}

void PowerMonitor::Task() {

}

int PowerMonitor::CommandsCb(int argc, char *argv[]) {

	return 0;
}


} // namespace bbque

