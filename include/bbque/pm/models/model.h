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


#ifndef BBQUE_MODEL_H_
#define BBQUE_MODEL_H_

#include <cstdint>
#include <string>

#define BBQUE_PM_DEFAULT_CRITICAL_TEMPERATURE 95

namespace bbque  { namespace pm {


/**
 * @class Model
 *
 * @brief Base class of the power-thermal model of hardware resources
 *
 * The class aims at providing a well-defined interface to power/thermal
 * resource allocation policy, hiding platform specific details.
 * According to the target platform, the framework should load the proper
 * resource models, provided through the implementation of suitable derived
 * classes.
 */
class Model {

public:

	/**
	 * @brief Constructor
	 * @param id Identifier string
	 * @param tpd Thermal-Power Design value in milliwatts
	 */
	Model(std::string id = "generic", uint32_t tpd = 100);

	/**
	 * @brief Model identifier
	 * @return Identifier string
	 */
	virtual std::string const & GetID();

	/**
	 * @brief Thermal-Power Design value
	 * @return Power value in milliwatts
	 */
	virtual uint32_t GetTPD();

	/**
	 * @brief The estimated power from the given power temperature
	 *
	 * @return Power in milliwatts
	 */
	virtual uint32_t GetPowerFromTemperature(uint32_t temp_mc);

	/**
	 * @brief The estimated temperature from the given power value
	 *
	 * @return Temperature in millidegree (Celsius)
	 */
	virtual uint32_t GetTemperatureFromPower(uint32_t power_mw);

	/**
	 * @brief The estimated percentage of resource utilization given a power
	 * value
	 *
	 * @return A floating point value in the range [0..1]
	 */
	virtual float GetResourcePercentageFromPower(uint32_t power_mw);

protected:

	std::string id;

	uint32_t tpd;

};

} // namespace pm

} // namespace bbque

#endif // BBQUE_MODEL_H_


