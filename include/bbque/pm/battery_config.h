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

 #include "bbque/config.h"

#ifndef BBQUE_BATTERY_CONFIG_H_
#define BBQUE_BATTERY_CONFIG_H_

/************ SysFS interfaces **********************/

#define BBQUE_BATTERY_SYS_ROOT         "/sys/class/power_supply/"
#define BBQUE_BATTERY_IF_TECHNOLOGY    "technology"

#ifndef CONFIG_BBQUE_PM_BATTERY_NOACPI
  #define BBQUE_BATTERY_IF_CHARGE_FULL   "charge_full"
  #define BBQUE_BATTERY_IF_CHARGE_MAH    "charge_now"
#else
  #define BBQUE_BATTERY_IF_CHARGE_FULL   "energy_full"
  #define BBQUE_BATTERY_IF_CHARGE_MAH    "energy_now"
#endif

#define BBQUE_BATTERY_IF_CHARGE_PERC   "capacity"
#define BBQUE_BATTERY_IF_STATUS        "status"

#define BBQUE_BATTERY_IF_CURRENT_NOW   "current_now"
#define BBQUE_BATTERY_IF_VOLTAGE_NOW   "voltage_now"
#define BBQUE_BATTERY_IF_POWER_NOW     "power_now"

// Status strings
#define BBQUE_BATTERY_STATUS_DIS       "Discharging"
#define BBQUE_BATTERY_STATUS_CHR       "Charging"
#define BBQUE_BATTERY_STATUS_FULL      "Full"


/************ ACPI interfaces **********************/

#define BBQUE_BATTERY_PROC_ROOT        "/proc/acpi/battery/"
#define BBQUE_BATTERY_IF_PROC_STATE    "state"
#define BBQUE_BATTERY_PROC_STATE_RATE  "present rate:"
#define BBQUE_BATTERY_PROC_STATE_VOLT  "present voltage:"

#endif // BBQUE_BATTERY_CONFIG_H_


