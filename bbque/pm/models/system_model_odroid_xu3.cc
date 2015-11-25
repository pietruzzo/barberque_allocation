 /*
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

#include "bbque/pm/models/system_model_odroid_xu3.h"


namespace bbque  { namespace pm {

/*
	--- CPUfreq = ondemand ---
	P_res = f(P_sys): [ -0.005  0.098  0.0876 -0.300 ]
	--- CPUfreq = performance ---
	P_res = f(P_sys): [ -0.005  0.137 -0.489  1.658 ]
*/

uint32_t ODROID_XU3_SystemModel::GetResourcePowerFromSystem(
		uint32_t p_mw,
		std::string const & freq_governor) const {
	double x = p_mw / 1e3;
	// performance
	if (freq_governor.compare(0, 3, "per") == 0)
		return (-0.005*pow(x,3) + 0.14*pow(x,2) - 0.49*x + 1.66) * 1e3;
	// ondemand
	return (-0.005*pow(x,3) + 0.10*pow(x,2) + 0.09*x - 0.30) * 1e3;
}


} // namespace pm

} // namespace bbque
