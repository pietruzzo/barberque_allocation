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
	uint32_t num_tiles[3]; // 0: total, 1:X, 2:Y

	// Get the number of clusters
	int err = hn_get_num_clusters(&this->num_clusters);
	if ( HN_SUCCEEDED != err ) {
		logger->Fatal("MangoPowerMonitor: unable to get the number of clusters "
			"[error=%d]", err);
	}

	hn_stats_monitor_t * curr_stats[2];

	// Initialize per-cluster tile info...
	for (uint32_t cluster_id=0; cluster_id < this->num_clusters; ++cluster_id) {
		int err = hn_get_num_tiles(&num_tiles[0], &num_tiles[1], &num_tiles[2], cluster_id);
		if (err == HN_SUCCEEDED) {
			tiles_info[cluster_id].resize(num_tiles[0]);
			tiles_stats[cluster_id].resize(num_tiles[0]);
			logger->Info("MangoPowerMonitor: cluster=%d includes %d tiles",
				cluster_id, num_tiles[0]);
		}
		else {
			logger->Fatal("MangoPowerMonitor: number of MANGO tiles missing [err=%d].", err);
			return;
		}

		for (uint_fast32_t tile_id = 0; tile_id < num_tiles[0]; ++tile_id) {

			// Static information
			int err = hn_get_tile_info(tile_id, &tiles_info[cluster_id][tile_id], cluster_id);
			if (HN_SUCCEEDED != err) {
				logger->Fatal("MangoPowerMonitor: unable to get info for "
					"cluster=<%d> tile=<%d> [error=%d].",
					cluster_id, tile_id, err);
				return;
			}

			// First counters values for initialization
			uint32_t nr_cores = 0;
			err = hn_stats_monitor_read(tile_id, &nr_cores, curr_stats, cluster_id);
			if (err == 0 && (curr_stats != nullptr)) {
				// Per tile...
				tiles_stats[cluster_id][tile_id].resize(nr_cores);
				logger->Debug("MangoPowerMonitor: cluster=<%d> tile=<%d> nr_cores=%d",
						cluster_id, tile_id, nr_cores);

				// Per core...
				for (uint32_t core_id = 0; core_id < nr_cores; ++core_id) {
					logger->Debug("MangoPowerMonitor: cluster=<%d> tile=<%d> core=<&d>"
						" initialization...",
						cluster_id, tile_id, core_id);
					tiles_stats[cluster_id][tile_id][core_id] = *curr_stats[core_id];
					free(curr_stats[core_id]);
				}
			}
			else {
				nr_cores = 1;
				tiles_stats[cluster_id][tile_id].resize(nr_cores);
				logger->Warn("MangoPowerMonitor: cluster=<%d> tile=<%d> nr_cores=%d fake",
						cluster_id, tile_id, nr_cores);
			}
		}
	}
}


PowerManager::PMResult
MangoPowerManager::GetLoad(ResourcePathPtr_t const & rp, uint32_t & perc) {

	// HN cluster
	int32_t cluster_id = rp->GetID(br::ResourceType::GROUP);
	if (cluster_id < 0) {
		logger->Warn("GetLoad: no cluster ID, using 0 by default");
		cluster_id = 0;
	}

	// Tile
	int tile_id = rp->GetID(br::ResourceType::ACCELERATOR);
	if (tile_id < 0) {
		logger->Warn("GetLoad: no tiles of type ACCELERATOR");
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	// Core
	int core_id = rp->GetID(br::ResourceType::PROC_ELEMENT);
	if (core_id < 0) {
		logger->Warn("GetLoad: no id for PROC_ELEMENT");
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	// Architecture type
	switch (tiles_info[cluster_id][tile_id].unit_family) {

	case HN_TILE_FAMILY_PEAK:
		logger->Debug("GetLoad: cluster=<%d> tile=<%d> core=<%d> is a PEAK processor",
			cluster_id, tile_id, core_id);
		return GetLoadPEAK(cluster_id, tile_id, core_id, perc);

	case HN_TILE_FAMILY_NUPLUS:
		logger->Debug("GetLoad: cluster=<%d> tile=<%d> is a NUPLUS processor",
			cluster_id, tile_id);
		return GetLoadNUP(cluster_id, tile_id, perc);
/*
	case HN_TILE_FAMILY_DCT:
		logger->Debug("GetLoad: cluster=<%d> tile=<%d> is a DCT accelerator",
			cluster_id, tile_id);
		hn_dct_get_utilization(tile_id, &perc, cluster_id);
		break;
	case HN_TILE_FAMILY_TETRAPOD:
		logger->Debug("GetLoad: cluster=<%d> tile=<%d> is a TETRAPOD accelerator",
			cluster_id, tile_id);
		hn_tetrapod_get_utilization(tile_id, &perc, cluster_id);
		break;
*/
	default:
		logger->Debug("GetLoad: cluster=<%d> tile=<%d> family=%s",
			cluster_id, tile_id,
			hn_to_str_unit_family(tiles_info[cluster_id][tile_id].unit_family));
		perc = 0;
	}
	return PMResult::OK;
}


PowerManager::PMResult
MangoPowerManager::GetLoadPEAK(
		uint32_t cluster_id,
		uint32_t tile_id,
		uint32_t core_id,
		uint32_t & perc) {

	uint32_t nr_cores = 0;
	hn_stats_monitor_t * curr_stats[2];

	uint32_t err = hn_stats_monitor_read(tile_id, &nr_cores, curr_stats, cluster_id);
	if (err == 0 && (curr_stats != nullptr)) {

		float cycles_ratio =
			float(curr_stats[core_id]->core_cycles - tiles_stats[cluster_id][tile_id][core_id].core_cycles) /
			       (curr_stats[core_id]->timestamp - tiles_stats[cluster_id][tile_id][core_id].timestamp);

		tiles_stats[cluster_id][tile_id][core_id] = *curr_stats[core_id];
		perc = cycles_ratio * 100;
		logger->Debug("GetLoadPEAK: cluster=<%d> tile=<%d> core=<%d>: "
			"ts=%ld tics_sleep=%d core_cycles=%d load=%d",
			cluster_id, tile_id, core_id,
			curr_stats[core_id]->timestamp,
			curr_stats[core_id]->tics_sleep,
			curr_stats[core_id]->core_cycles,
			perc);
		free(curr_stats[core_id]);
	}
	else {
		perc = 0;
		logger->Error("GetLoadPEAK: cluster=<%d> tile=<%d> [error=%d]",
			cluster_id, tile_id, err);
	}

	return PMResult::OK;
}

PowerManager::PMResult
MangoPowerManager::GetLoadNUP(
		uint32_t cluster_id,
		uint32_t tile_id,
		uint32_t & perc) {

	hn_stats_monitor_t * curr_stats = new hn_stats_monitor_t;
	uint32_t err = hn_nuplus_stats_read(tile_id, curr_stats, cluster_id);
	if (err == 0 && (curr_stats != nullptr)) {
		perc = (uint32_t) curr_stats->core_cpi;
		logger->Debug("GetLoadNUP: cluster=<%d> tile=<%d>: "
			"ts=%ld tics_sleep=%d core_cycles=%d load=%d",
			cluster_id,
			tile_id,
			curr_stats->timestamp,
			curr_stats->tics_sleep,
			curr_stats->core_cycles,
			perc);
		tiles_stats[cluster_id][tile_id][0] = *curr_stats;
	}
	else {
		perc = 0;
		logger->Error("GetLoadNUP: cluster=<%d> tile=<%d> [error=%d]",
			cluster_id, tile_id, err);
	}

	delete curr_stats;
	return PMResult::OK;
}


PowerManager::PMResult
MangoPowerManager::GetTemperature(ResourcePathPtr_t const & rp, uint32_t &celsius) {

	// HN cluster
	int32_t cluster_id = rp->GetID(br::ResourceType::GROUP);
	if (cluster_id < 0) {
		logger->Warn("GetTemperature: no cluster ID, using 0 by default");
		cluster_id = 0;
	}

	// Tile
	int tile_id = rp->GetID(br::ResourceType::ACCELERATOR);
	if (tile_id < 0) {
		logger->Warn("GetTemperature: no tiles of type ACCELERATOR");
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	float temp = 0;
	int err = hn_get_tile_temperature(tile_id, &temp, cluster_id);
	if (err != 0) {
		logger->Error("GetTemperature: cluster=<%d> tile=<%d>  [error=%d]",
			cluster_id, tile_id, err);
		return PMResult::ERR_UNKNOWN;
	}

	celsius = static_cast<uint32_t>(temp);
	return PMResult::OK;
}


PowerManager::PMResult
MangoPowerManager::GetClockFrequency(ResourcePathPtr_t const & rp, uint32_t &khz) {

	// HN cluster
	int32_t cluster_id = rp->GetID(br::ResourceType::GROUP);
	if (cluster_id < 0) {
		logger->Warn("GetClockFrequency: no cluster ID, using 0 by default");
		cluster_id = 0;
	}

	// Tile
	int tile_id = rp->GetID(br::ResourceType::ACCELERATOR);
	if (tile_id < 0) {
		logger->Warn("GetClockFrequency: no tiles of type ACCELERATOR");
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	khz = tiles_stats[cluster_id][tile_id][0].frequency / 1e3;
	return PMResult::OK;
}


PowerManager::PMResult
MangoPowerManager::GetPowerUsage(br::ResourcePathPtr_t const & rp, uint32_t & mwatt) {

	// HN cluster
	int32_t cluster_id = rp->GetID(br::ResourceType::GROUP);
	if (cluster_id < 0) {
		logger->Warn("GetPowerUsage: no cluster ID, using 0 by default");
		cluster_id = 0;
	}

	// Tile
	int tile_id = rp->GetID(br::ResourceType::ACCELERATOR);
	if (tile_id < 0) {
		logger->Warn("GetPowerUsage: no tiles of type ACCELERATOR");
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	// Core
	int core_id = rp->GetID(br::ResourceType::PROC_ELEMENT);
	if (core_id < 0) {
		logger->Warn("GetLoad: no id for PROC_ELEMENT");
		return PMResult::ERR_RSRC_INVALID_PATH;
	}

	logger->Info("GetPowerUsage: cluster=<%d> tile=<%d> [%s]: "
		"power_factor=%.2f perf_factor=%.2f",
		cluster_id, tile_id,
		hn_to_str_unit_family(tiles_info[cluster_id][tile_id].unit_family),
		tiles_info[cluster_id][tile_id].power_factor,
		tiles_info[cluster_id][tile_id].performance_factor);

	// Architecture type
	switch (tiles_info[cluster_id][tile_id].unit_family) {

	case HN_TILE_FAMILY_NUPLUS:
		logger->Debug("GetPowerUsage: cluster=<%d> tile=<%d>: NU+ processor",
			cluster_id, tile_id);
		mwatt = static_cast<uint32_t>(tiles_stats[cluster_id][tile_id][core_id].power_est);
		return PMResult::OK;

	case HN_TILE_FAMILY_PEAK:
		mwatt = static_cast<uint32_t>(tiles_info[cluster_id][tile_id].power_factor * 1e3);
		return PMResult::OK;

	default:
		mwatt = static_cast<uint32_t>(tiles_info[cluster_id][tile_id].power_factor * 1e3);
		return PMResult::OK;
	}

	return PMResult::OK;
}


} // namespace bbque

