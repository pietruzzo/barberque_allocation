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
#ifndef BBQUE_POWER_MANAGER_ODROID_H_
#define BBQUE_POWER_MANAGER_ODROID_H_

#include "bbque/config.h"

namespace bbque {

/***********************************************
 * Odroid XU-3 board
 ***********************************************/
#ifdef CONFIG_TARGET_ODROID_XU

// Drivers directory
#define BBQUE_ODROID_SYS_DIR_GPU\
	"/sys/bus/platform/drivers/mali/11800000.mali/"
// Sensors
#define BBQUE_ODROID_SENSORS_DIR_A7    "/sys/bus/i2c/drivers/INA231/3-0041/"
#define BBQUE_ODROID_SENSORS_DIR_A15   "/sys/bus/i2c/drivers/INA231/3-0040/"
#define BBQUE_ODROID_SENSORS_DIR_GPU   "/sys/bus/i2c/drivers/INA231/3-0044/"
#define BBQUE_ODROID_SENSORS_DIR_MEM   "/sys/bus/i2c/drivers/INA231/3-0045/"

#define BBQUE_ODROID_SENSORS_TEMP          "/sys/devices/10060000.tmu/temp"
#define BBQUE_ODROID_SENSORS_OFFSET_A15_0  10
#define BBQUE_ODROID_SENSORS_OFFSET_A15_1  16
#define BBQUE_ODROID_SENSORS_OFFSET_A15_2  22
#define BBQUE_ODROID_SENSORS_OFFSET_A15_3  28
#define BBQUE_ODROID_SENSORS_OFFSET_GPU    34


// Attributes
#define BBQUE_ODROID_SENSORS_ATT_V     "sensor_V"
#define BBQUE_ODROID_SENSORS_ATT_W     "sensor_W"
#define BBQUE_ODROID_SENSORS_ATT_A     "sensor_A"

#endif // TARGET_ODROID_XU

/***********************************************
 * Odroid XU-E board
 ***********************************************/

} // namespace bbque

#endif // BBQUE_POWER_MANAGER_ODROID_H_
