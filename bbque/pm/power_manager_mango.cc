/*
 * Copyright (C) 2018  Politecnico di Milano
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

#include <cstring>

#include "bbque/pm/power_manager_mango.h"

using namespace bbque::res;

namespace bbque {

MangoPowerManager::MangoPowerManager() {
	logger->Info("MangoPowerManager initialization...");
	memset(&tile_stats, 0, sizeof(hn_tile_stats_t));
}

PowerManager::PMResult
MangoPowerManager::GetLoad(ResourcePathPtr_t const & rp, uint32_t & perc) {
	uint32_t tile_id = rp->GetID(br::ResourceType::ACCELERATOR);
	if (hn_get_tile_stats(tile_id, &tile_stats) != 0) {
		logger->Warn("GetLoad: error in HN library call...");
		return PMResult::ERR_UNKNOWN;
	}
	perc = tile_stats.unit_utilization;
	return PMResult::OK;
}


PowerManager::PMResult
MangoPowerManager::GetTemperature(ResourcePathPtr_t const & rp, uint32_t &celsius) {
	uint32_t tile_id = rp->GetID(br::ResourceType::ACCELERATOR);
	float temp = 0;
	if (hn_get_tile_temperature(tile_id, &temp) != 0) {
		logger->Warn("GetTemperature: error in HN library call...");
		return PMResult::ERR_UNKNOWN;
	}
	celsius = static_cast<uint32_t>(temp);
	return PMResult::OK;
}


} // namespace bbque

