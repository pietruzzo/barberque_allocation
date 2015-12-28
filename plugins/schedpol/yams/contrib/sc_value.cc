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

#include <cmath>

#include "sc_value.h"

namespace ba = bbque::app;
namespace br = bbque::res;
namespace po = boost::program_options;

namespace bbque { namespace plugins {


SCValue::SCValue(
		const char * _name,
		BindingInfo_t const & _bd_info,
		uint16_t const cfg_params[]):
	SchedContrib(_name, _bd_info, cfg_params) {
	char conf_str[50];
	uint16_t napw;
	po::options_description opts_desc("AWM value contribution parameters");

	// NAP weight
	snprintf(conf_str, 50, SC_CONF_BASE_STR"%s.nap.weight", name);
	opts_desc.add_options()
		(conf_str,
		 po::value<uint16_t>(&napw)->default_value(SC_VALUE_NAPW_DEFAULT),
		 "Normalized Actual Penalty (NAP) weight");
		;
	po::variables_map opts_vm;
	cm.ParseConfigurationFile(opts_desc, opts_vm);

	// Check boundary value
	if (napw > 100) {
		napw = SC_VALUE_NAPW_DEFAULT;
		logger->Warn("NAP weight out of range (set to default = %.2f)",
				(float) SC_VALUE_NAPW_DEFAULT / 100.0);
	}

	// Convent to a indexed value [0..1]
	nap_weight = static_cast<float>(napw) / 100.0;
	logger->Info("Goal-gap weight \t= %.2f", nap_weight);
}

SCValue::~SCValue() {
}

SchedContrib::ExitCode_t
SCValue::Init(void * params) {
	(void) params;

	return SC_SUCCESS;
}

SchedContrib::ExitCode_t
SCValue::_Compute(SchedulerPolicyIF::EvalEntity_t const & evl_ent,
		float & ctrib) {
	ba::AwmPtr_t const & curr_awm(evl_ent.papp->CurrentAWM());

	// Initialize the index contribute to the AWM static value
	logger->Debug("%s: Static value = %.2f ",
			evl_ent.StrId(), evl_ent.pawm->Value());
	if (!curr_awm || (evl_ent.papp->GetGoalGap() == 0)) {
		ctrib = evl_ent.pawm->Value();
		return SC_SUCCESS;
	}

	// Negative Goal-Gap => Over-performance  - AWM to be decreased
	// Positive Goal-Gap => Under-performance - AWM to be increased
	// Compute an "ideal" AWM value, scaling the current AWM by the value of
	// the Goal-Gap.
	float weight;
	float goal_gap_perc = static_cast<float>(evl_ent.papp->GetGoalGap()) / 100.0;
	goal_gap_perc > 0 ? weight = 1: weight = nap_weight;
	float ideal_value   = curr_awm->Value() * (1 + (weight * 1/(1+goal_gap_perc)));
	logger->Debug("%s: Gap=%.2f, currV=%.2f, evalV=%2f, idealV=%.2f, dV=%.2f",
			evl_ent.StrId(), goal_gap_perc,
			curr_awm->Value(), evl_ent.pawm->Value(), ideal_value,
			static_cast<float>(evl_ent.pawm->Value()) - ideal_value);

	// Get the delta between the value of the AWM under evaluation and the
	// value of the ideal AWM value. The evaluated AWM with the value closest
	// to the "ideal" one estimated is promoted
	float delta = std::abs(static_cast<float>(evl_ent.pawm->Value()) - ideal_value);
	ctrib = 1.0 - std::min<float>(1.0, delta);
	logger->Debug("%s: AWM Value index: %.4f", evl_ent.StrId(), ctrib);

	return SC_SUCCESS;
}


} // namespace plugins

} // namespace bbque

