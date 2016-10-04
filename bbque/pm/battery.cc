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

#include <cassert>
#include <cctype>
#include <cstring>
#include <boost/filesystem.hpp>

#include "bbque/pm/battery.h"
#include "bbque/utils/iofs.h"

#define MODULE_NAMESPACE "bq.pm.bat"

namespace bu = bbque::utils;

namespace bbque {


Battery::Battery(
		std::string const & name,
		const char * i_dir,
		const char * s_dir):
	str_id(name),
	info_dir(i_dir),
	status_dir(s_dir) {
	// Logger
	std::string logname(MODULE_NAMESPACE);
	logname.append("." + str_id);
	logger = bu::Logger::GetLogger(logname.c_str());
	assert(logger);

	// Directories
	info_dir.append("/" + str_id + "/");
	if (!boost::filesystem::exists(info_dir)) {
		logger->Error("Missing directory: %s", info_dir.c_str());
		return;
	}
	logger->Debug("Directory %s found", info_dir.c_str());


#ifndef CONFIG_BBQUE_PM_NOACPI
	status_dir.append("/" + str_id + "/");
	if (!boost::filesystem::exists(status_dir)) {
		logger->Error("Missing directory: %s", status_dir.c_str());
		return;
	}
#endif // CONFIG_BBQUE_PM_NOACPI

	// Technology string
	char t_str[12];
	memset(t_str, '\0', sizeof(t_str));
	logger->Debug("Battery: reading from %s", BBQUE_BATTERY_IF_TECHNOLOGY);
	bu::IoFs::ReadValueFrom(info_dir + BBQUE_BATTERY_IF_TECHNOLOGY,
			t_str, sizeof(t_str)-1);
	technology = t_str;
	logger->Debug("Technology: %s", t_str);

	// Full capacity
	std::string charge_full_file("");
	if (boost::filesystem::exists(info_dir + BBQUE_BATTERY_IF_CHARGE_FULL))
		charge_full_file = info_dir + BBQUE_BATTERY_IF_CHARGE_FULL;
	else if (boost::filesystem::exists(info_dir + BBQUE_BATTERY_IF_ENERGY_FULL))
		charge_full_file = info_dir + BBQUE_BATTERY_IF_ENERGY_FULL;
	else
		logger->Error("Batter: missing input for charge full status");
	logger->Debug("Battery: reading from %s", charge_full_file.c_str());

	char cf_str[20];
	memset(cf_str, '\0', sizeof(cf_str));
	bu::IoFs::ReadValueFrom(charge_full_file, cf_str, sizeof(cf_str)-1);
	logger->Debug("Charge full: %s", cf_str);
	if (strlen(cf_str) > 0)
		charge_full = std::stol(cf_str) / 1000;

	// Charge status (mAh) file
	if (boost::filesystem::exists(info_dir + BBQUE_BATTERY_IF_CHARGE_MAH))
		charge_mah_file = info_dir + BBQUE_BATTERY_IF_CHARGE_MAH;
	else if (boost::filesystem::exists(info_dir + BBQUE_BATTERY_IF_ENERGY_MAH))
		charge_mah_file = info_dir + BBQUE_BATTERY_IF_ENERGY_MAH;
	else
		logger->Error("Battery: missing input for status");
	logger->Debug("Battery: charge level (mAh) file = %s", charge_mah_file.c_str());

	// Power now
	if (boost::filesystem::exists(info_dir + BBQUE_BATTERY_IF_POWER_NOW)) {
		power_now_exists = true;
		logger->Debug("Battery: instant power input available");
	}

	// Status report
	ready = true;
	LogReportStatus();
}

bool Battery::IsReady() const {
	return ready;
}

std::string const & Battery::StrId() const {
	return str_id;
}

std::string const & Battery::GetTechnology() const {
	return technology;
}

unsigned long Battery::GetChargeFull() const {
	return charge_full;
}

bool Battery::IsDischarging() {
	char status[13];
	bu::IoFs::ReadValueFrom(info_dir + BBQUE_BATTERY_IF_STATUS,
			status, sizeof(status)-1);
	return (strncmp(status, BBQUE_BATTERY_STATUS_DIS, 3) == 0);
}

inline uint32_t Battery::GetMilliUInt32From(std::string const & path) {
	bu::IoFs::ExitCode_t result;
	uint32_t m_value;

	if (!boost::filesystem::exists(path)) {
		logger->Error("Missing file: %s", path.c_str());
		return 0;
	}

	result = bu::IoFs::ReadIntValueFrom<uint32_t>(path, m_value);
	if (result != bu::IoFs::OK) {
		logger->Error("Failed in reading %s", path.c_str());
		return 0;
	}
	return m_value / 1e3;
}

uint8_t Battery::GetChargePerc() {
	uint8_t perc = 0;
	char perc_str[4];

	bu::IoFs::ReadValueFrom(info_dir + BBQUE_BATTERY_IF_CHARGE_PERC,
			perc_str, sizeof(perc_str)-1);
	if (!isdigit(perc_str[0])) {
		logger->Error("Not valid charge value");
		return 0;
	}

	perc = std::stoi(perc_str);
	if (perc > 100) {
		logger->Error("Charge value > 100 %%");
		return 0;
	}
	return perc;
}

unsigned long Battery::GetChargeMAh() {
	char mah[10];

	bu::IoFs::ReadValueFrom(charge_mah_file, mah, sizeof(mah)-1);
	if (!isdigit(mah[0])) {
		logger->Error("Not valid charge value");
		return 0;
	}
	return std::stol(mah) / 1000;
}

uint32_t Battery::GetVoltage() {
#ifndef CONFIG_BBQUE_PM_NOACPI
	return ACPI_GetVoltage();
#else
	return GetMilliUInt32From(info_dir + BBQUE_BATTERY_IF_VOLTAGE_NOW);
#endif
}

uint32_t Battery::GetPower() {
	if (power_now_exists)
		return GetMilliUInt32From(info_dir + BBQUE_BATTERY_IF_POWER_NOW);
	else
		return (GetDischargingRate() * GetVoltage()) / 1e3;
}

uint32_t Battery::GetDischargingRate() {
#ifndef CONFIG_BBQUE_PM_NOACPI
	return ACPI_GetDischargingRate();
#else
	return GetMilliUInt32From(info_dir + BBQUE_BATTERY_IF_CURRENT_NOW);
#endif
}


unsigned long Battery::GetEstimatedLifetime() {
	float hours = (float) GetChargeMAh() / (float) GetDischargingRate();
	logger->Debug("Est. time   : \thours=%.2f (min=%.2f)",
			hours, hours * 60);
	return static_cast<unsigned long>(hours * 3600);
}

void Battery::LogReportStatus() {
	logger->Info("Technology  : \t%s",      technology.c_str());
	logger->Info("Full Charge : \t%lu mAh", charge_full);
	logger->Info("Level       : \t%s",      PrintChargeBar().c_str());
	logger->Info("Charge      : \t%lu mAh", GetChargeMAh());
	logger->Info("Rate        : \t%d mA",   GetDischargingRate());
	logger->Info("Voltage     : \t%lu mV",  GetVoltage());
	logger->Info("Est. time   : \t%lu s",   GetEstimatedLifetime());
}

std::string Battery::PrintChargeBar() {
	int i;
	int charge = GetChargePerc();
	std::string bar("[");
	for (i = 0; i < (charge / 10); ++i)
		bar += "=";
	bar += "] ";
	bar += std::to_string(charge);
	bar += "%";
	return bar;
}

/********************** ACPI dependent code ***************************/

#ifndef CONFIG_BBQUE_PM_NOACPI

uint32_t Battery::ACPI_GetVoltage() {
	char volt[7];
	bu::IoFs::ParseValue(status_dir + BBQUE_BATTERY_IF_PROC_STATE,
			BBQUE_BATTERY_PROC_STATE_VOLT, volt, sizeof(volt)-1);
	if (!isdigit(volt[0])) {
		logger->Error("Not valid voltage value");
		return 0;
	}
	return std::stoi(volt);
}

uint32_t Battery::ACPI_GetDischargingRate() {
	if (!IsDischarging()) {
		return 0;
	}

	char rate[7];
	bu::IoFs::ParseValue(status_dir + BBQUE_BATTERY_IF_PROC_STATE,
			BBQUE_BATTERY_PROC_STATE_RATE, rate, sizeof(rate)-1);
	if (!isdigit(rate[0])) {
		logger->Error("Not valid discharging rate");
		return 0;
	}
	return std::stoi(rate);
}

#endif // CONFIG_BBQUE_PM_NOACPI



} // namespace bbque

