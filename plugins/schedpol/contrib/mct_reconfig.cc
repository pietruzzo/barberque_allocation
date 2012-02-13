/*
 * Copyright (C) 2012  Politecnico di Milano
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

#include "mct_reconfig.h"

namespace po = boost::program_options;

namespace bbque { namespace plugins {


MCTReconfig::MCTReconfig(const char * _name, uint16_t cfg_params[]):
	MetricsContribute(_name, cfg_params) {
	char conf_str[40];

	// Configuration parameters
	po::options_description opts_desc("Reconfiguration contribute params");
	snprintf(conf_str, 40, "MetricsContribute.%s.migfact", name);

	opts_desc.add_options()
		(conf_str,
		 po::value<uint16_t>
		 (&migfact)->default_value(DEFAULT_MIGRATION_FACTOR),
		 "Migration factor");
		;

	po::variables_map opts_vm;
	cm.ParseConfigurationFile(opts_desc, opts_vm);
	logger->Debug("Migration factor: %d", migfact);
}

MetricsContribute::ExitCode_t
MCTReconfig::_Compute(EvalEntity_t const & evl_ent, float & ctrib) {
	UsagesMap_t::const_iterator usage_it;
	float reconf_cost = 0.0;
	uint8_t to_mig    = 0;
	uint64_t rsrc_avl = 0.0;
	uint64_t rsrc_tot;

	// Check if a migration would be required, if yes enable the factor
	if (evl_ent.papp->CurrentAWM() &&
			!evl_ent.papp->CurrentAWM()->ClusterSet().test(evl_ent.clust_id)) {
		to_mig = 1;
		uint32_t clset = evl_ent.papp->CurrentAWM()->ClusterSet().to_ulong();
		logger->Debug("%s: current CLs:{%d} => MIG:%d", evl_ent.StrId(),
				uint32_t(log(clset)/log(2)), to_mig);
	}

	// Resource usages of the current entity (AWM + Cluster)
	for_each_sched_resource_usage(evl_ent, usage_it) {
		std::string const & rsrc_path(usage_it->first);
		UsagePtr_t const & pusage(usage_it->second);
		ResourcePtrList_t & rsrc_bind(pusage->GetBindingList());

		// Query resource availability
		rsrc_avl = sv->ResourceAvailable(rsrc_bind, vtok, evl_ent.papp);
		if (rsrc_avl < pusage->GetAmount()) {
			logger->Warn("%s: {%s} RQ:%llu| AVL:%llu", evl_ent.StrId(),
					rsrc_path.c_str(), pusage->GetAmount(), rsrc_avl);
			// Resource allocation is completely discouraged
			ctrib = 0.0;
			if ((rsrc_avl == 0) &&
					(ResourcePathUtils::GetNameTemplate(rsrc_path).compare("pe")
					 == 0))
				return MCT_RSRC_NO_PE;
			return MCT_RSRC_UNAVL;
		}

		// Total amount of resource
		rsrc_tot = sv->ResourceTotal(pusage->GetBindingList());
		logger->Debug("%s: {%s} RQ:%llu| AVL:%llu| TOT:%llu", evl_ent.StrId(),
				rsrc_path.c_str(), pusage->GetAmount(), rsrc_avl, rsrc_tot);

		// Reconfiguration cost
		reconf_cost += ((float) pusage->GetAmount() / (float) rsrc_tot);
	}

	// Contribute value
	ctrib = 1.0 - (1.0 + (float) to_mig * migfact) / (1.0 + (float) migfact) *
		((float) reconf_cost / sv->GetNumResourceTypes());

	return MCT_SUCCESS;
}


} // namespace plugins

} // namespace bbque

