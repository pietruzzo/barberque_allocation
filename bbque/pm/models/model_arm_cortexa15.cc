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
#include <cstring>

namespace bbque  { namespace pm {

/*
   --- CPUfreq = ondemand ---
   T = f(P):  [  0.36515765   3.19384347  51.80211498]
   P = f(T):  [ -1.02236632e-03   2.94065642e-01  -1.27568814e+01]
   C = f(P):  [ -0.19491434   2.03944065  40.9271282   27.69173536]

   --- CPUfreq = performance ---
   T = f(P):  [ -0.91933073  15.69965691  33.3279612 ]
   P = f(T):  [ -5.69142325e-04   1.86787028e-01  -7.41306315e+00]
   C = f(P):  [   2.20101853  -23.81482262  120.31158665 -117.61752503]
*/

uint32_t ARM_CortexA15_Model::GetPowerFromTemperature(
		uint32_t temp_mc,
		std::string const & freq_governor) {
	double x = temp_mc / 1e3;
	// OLD:
//	return (2.19e-03*pow(x,2) - 2.29e-01*(x) + 8.48) * 1e3;
	// performance
	if (freq_governor.compare(0, 3, "per") == 0)
		return (-5.69e-04*pow(x,2) + 1.87*(x) - 7.41) * 1e3;
	// ondemand
	return (-1.02e-03*pow(x,2) + 2.94e-01*(x) - 1.28e+01) * 1e3;
}

uint32_t ARM_CortexA15_Model::GetPowerFromSystemBudget(
		uint32_t power_mw,
		std::string const & freq_governor) {
	(void) freq_governor;
	double x = power_mw * 0.9;
	return static_cast<uint32_t>(x);
}

uint32_t ARM_CortexA15_Model::GetTemperatureFromPower(
		uint32_t power_mw,
		std::string const & freq_governor) {
	(void) power_mw;
	(void) freq_governor;
	return 0;
}

float ARM_CortexA15_Model::GetResourcePercentageFromPower(
		uint32_t power_mw,
		std::string const & freq_governor) {
	(void) freq_governor;
	double x = power_mw / 1e3;
	// OLD:
	return (0.0116 * pow(x,2) + 0.057*(x) + 0.018);
}

uint32_t ARM_CortexA15_Model::GetResourceFromPower(
		uint32_t power_mw,
		uint32_t total_amount,
		std::string const & freq_governor) {
	(void) total_amount;
	double x = power_mw / 1e3;
	// performance
	if (freq_governor.compare(0, 3, "per") == 0)
		return (2.20*pow(x,3) - 23.81*pow(x,2) + 120.31*(x) - 117.62);
	// ondemand
	return (-0.19*pow(x,3) + 2.04*pow(x,2) + 40.93*(x) + 27.69);
}

} // namespace pm

} // namespace bbque
