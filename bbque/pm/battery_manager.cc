/*
 * Copyright (C) 2015  Politecnico di Milano
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

#include <boost/filesystem.hpp>

#include "bbque/pm/battery_manager.h"

#define MODULE_NAMESPACE "bq.bm"


namespace bbque {

BatteryManager & BatteryManager::GetInstance() {
	static BatteryManager instance;
	return instance;
}

BatteryManager::BatteryManager() {
	logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	assert(logger);
	logger->Info("BatteryManager initialization...");

	// Check the directories
	if (!boost::filesystem::exists(BBQUE_BATTERY_SYS_ROOT)) {
		logger->Error("PWR MNT: No batteries");
		return;
	}

	// Iterate over the subdirectories
	boost::filesystem::directory_iterator itr(BBQUE_BATTERY_SYS_ROOT);
	boost::filesystem::directory_iterator end_itr;
	for (; itr != end_itr; ++itr) {
		if (!boost::filesystem::is_directory(itr->status()))
			continue;

		std::string dir_prefix(itr->path().filename().string());
		dir_prefix = dir_prefix.substr(0,3);
		if ((dir_prefix.compare("BAT") != 0) &&
			(dir_prefix.compare("bat") != 0)) continue;

		BatteryPtr_t batt(new Battery(itr->path().filename().c_str()));
		if (!batt->IsReady())
			continue;
		batteries.push_back(batt);
		logger->Info("Batteries in the system: %d", batteries.size());
	}
}

BatteryManager::~BatteryManager() {
	batteries.clear();
}

BatteryPtr_t BatteryManager::GetBattery(uint8_t id) {
	return batteries[id];
}

int BatteryManager::CommandsCb(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	return 0;
}

} // namespace bbque

