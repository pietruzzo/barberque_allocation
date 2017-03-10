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

#include "bbque/resource_mapping_validator.h"

#define MODULE_NAMESPACE   "bq.rmv"
#define MODULE_CONFIG      "ResourceMappingValidator"


namespace bbque {


ResourceMappingValidator & ResourceMappingValidator::GetInstance() {
	static ResourceMappingValidator instance;
	return instance;
}

ResourceMappingValidator::ResourceMappingValidator():
		logger(bbque::utils::Logger::GetLogger(MODULE_NAMESPACE)) {
	// Nothing to do
}

void ResourceMappingValidator::
		RegisterSkimmer(PartitionSkimmerPtr_t skimmer, int priority) noexcept {
		skimmers.insert(
			std::pair<int,PartitionSkimmerPtr_t> (priority, skimmer) 
		);
}


ResourceMappingValidator::ExitCode_t
ResourceMappingValidator::LoadPartitions(const TaskGraph &tg, std::list<Partition> &partitions) {

	logger->Notice("Initial partitions nr. %d", partitions.size());

	if ( skimmers.empty() ) {
		logger->Warn("No skimmers registered, no action performed.");
		return PMV_OK;
	}


	PartitionSkimmer::SkimmerType_t skimmer_type = PartitionSkimmer::SkimmerType_t::SKT_NONE;
	// I get the skimmers in the reverse order, in order to execute the one with highest
	// priority
	for (auto s = skimmers.rbegin(); s != skimmers.rend(); ++s) {
		int priority = s->first;
		PartitionSkimmerPtr_t skimmer = s->second; 
		skimmer_type = skimmer->GetType(); 

		logger->Debug("Executing skimmer [type=%d] [priority=%d]", (int)skimmer_type, priority);

		int ret = skimmer->Skim(tg, partitions);

		if (ret != 0) {
			logger->Error("Skimmer %d [priority=%d] FAILED [err=%d]", 
				(int)skimmer_type, priority, ret);
			this->failed_skimmer = skimmer_type;
			return PMV_SKIMMER_FAIL;
		}

		if ( partitions.empty() ) {
			break;
		}
	}

	if ( partitions.empty() ) {
		logger->Notice("Skimmer %d: no feasible partitions", 
				(int)skimmer_type);

		this->failed_skimmer = skimmer_type;
		// No feasible solution found
		return PMV_NO_PARTITION;
	}

	return PMV_OK;
}
} // namespace bbque

