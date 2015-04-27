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

#include "bbque/pm/models/model.h"

#include <cmath>

namespace bbque  { namespace pm {


Model::Model(std::string _id, uint32_t _tpd):
	id(_id),
	tpd(_tpd) {
}

std::string const & Model::GetID() {
	return id;
}

uint32_t Model::GetTPD() {
	return tpd;
}

uint32_t Model::GetPowerFromTemperature(uint32_t temp_mc) {
	(void) temp_mc;
	return tpd;
}

uint32_t Model::GetTemperatureFromPower(uint32_t power_mw) {
	(void) power_mw;
	return BBQUE_PM_DEFAULT_CRITICAL_TEMPERATURE;
}

float Model::GetResourcePercentageFromPower(uint32_t power_mw) {
	(void) power_mw;
	return 1.0;
}

} // namespace pm

} // namespace bbque

