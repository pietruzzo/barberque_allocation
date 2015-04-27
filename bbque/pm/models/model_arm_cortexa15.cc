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

#include "bbque/pm/models/model_arm_cortexa15.h"

#include <cmath>

namespace bbque  { namespace pm {


uint32_t ARM_CortexA15_Model::GetPowerFromTemperature(
		uint32_t t_mc) {
	return 2.19e-03 * pow(t_mc, 2.0) + (-2.29e-01 * t_mc) + 8.48e+00;
}

uint32_t ARM_CortexA15_Model::GetTemperatureFromPower(
		uint32_t p_mw) {
	(void) p_mw;
	return 0;
}

float ARM_CortexA15_Model::GetResourcePercentageFromPower(
		uint32_t p_mw) {
	return 0.011592 * pow(p_mw, 2.0) + (0.057145 * p_mw) + 0.018222;
}

} // namespace pm

} // namespace bbque
