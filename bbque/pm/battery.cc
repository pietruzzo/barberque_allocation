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
	status_dir.append("/" + str_id + "/");
	if (!boost::filesystem::exists(status_dir)) {
		logger->Error("Missing directory: %s", status_dir.c_str());
		return;
	}

	// Technology string
	char t_str[12];
	memset(t_str, '\0', sizeof(t_str));
	bu::IoFs::ReadValueFrom(info_dir + BBQUE_BATTERY_IF_TECHNOLOGY,
			t_str, sizeof(t_str)-1);
	technology = t_str;

	// Full capacity
	char cf_str[20];
	memset(cf_str, '\0', sizeof(cf_str));
	bu::IoFs::ReadValueFrom(info_dir + BBQUE_BATTERY_IF_CHARGE_FULL,
			cf_str, sizeof(cf_str)-1);
	charge_full = std::stol(cf_str) / 1000;

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
	memset(status, '\0', sizeof(status));
	bu::IoFs::ReadValueFrom(info_dir + BBQUE_BATTERY_IF_STATUS,
			status, sizeof(status)-1);
	return (strncmp(status, BBQUE_BATTERY_STATUS_DIS, 3) == 0);
}

uint8_t Battery::GetChargePerc() {
	uint8_t perc = 0;
	char perc_str[4];
	memset(perc_str, '\0', sizeof(perc_str));
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
	memset(mah, '\0', sizeof(mah));
	bu::IoFs::ReadValueFrom(info_dir + BBQUE_BATTERY_IF_CHARGE_MAH,
			mah, sizeof(mah)-1);
	if (!isdigit(mah[0])) {
		logger->Error("Not valid charge value");
		return 0;
	}
	return std::stol(mah) / 1000;
}

uint32_t Battery::GetVoltage() {
	char volt[7];
	memset(volt, '\0', sizeof(volt));
	bu::IoFs::ParseValue(status_dir + BBQUE_BATTERY_IF_PROC_STATE,
			BBQUE_BATTERY_PROC_STATE_VOLT, volt, sizeof(volt)-1);
	if (!isdigit(volt[0])) {
		logger->Error("Not valid voltage value");
		return 0;
	}
	return std::stoi(volt);
}

uint32_t Battery::GetPower() {
	return (GetDischargingRate() * GetVoltage()) / 1e3;
}

uint32_t Battery::GetDischargingRate() {
	if (!IsDischarging()) {
		return 0;
	}

	char rate[7];
	memset(rate, '\0', sizeof(rate));
	bu::IoFs::ParseValue(status_dir + BBQUE_BATTERY_IF_PROC_STATE,
			BBQUE_BATTERY_PROC_STATE_RATE, rate, sizeof(rate)-1);
	if (!isdigit(rate[0])) {
		logger->Error("Not valid discharging rate");
		return 0;
	}
	return std::stoi(rate);
}

unsigned long Battery::GetEstimatedLifetime() {
	float hours = (float) GetChargeMAh() / (float) GetDischargingRate();
	logger->Debug("Est. time   : \thours=%.2f (min=%.2f)",
			hours, hours * 60);
	return static_cast<unsigned long>(hours * 3600);
}

void Battery::LogReportStatus() {
	logger->Info("Technology  : \t%s", 	    technology.c_str());
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

} // namespace bbque

