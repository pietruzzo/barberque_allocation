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


#define MODULE_NAMESPACE "bq.pm.bat"


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
	char t_str[10];
	ReadValue(info_dir + BBQUE_BATTERY_IF_TECHNOLOGY, t_str, 10);
	t_str[strlen(t_str)-1] = '\0';
	technology = t_str;

	// Full capacity
	char cf_str[20];
	ReadValue(info_dir + BBQUE_BATTERY_IF_CHARGE_FULL, cf_str, 20);
	charge_full = std::stol(cf_str) / 1000;

	// Status report
	ready = true;
	LogReportStatus();
}

bool Battery::IsReady() {
	return ready;
}

std::string const & Battery::StrId() {
	return str_id;
}

std::string const & Battery::GetTechnology() {
	return technology;
}

unsigned long Battery::GetChargeFull() {
	return charge_full;
}

bool Battery::IsDischarging() {
	char status[12];
	ReadValue(info_dir + BBQUE_BATTERY_IF_STATUS, status, 12);
	status[strlen(status)-1] = '\0';
	return (strncmp(status, BBQUE_BATTERY_STATUS_DIS, 3) == 0);
}

uint8_t Battery::GetChargePerc() {
	char perc_str[3];
	uint8_t perc;
	ReadValue(info_dir + BBQUE_BATTERY_IF_CHARGE_PERC, perc_str, 3);
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
	char mah[9];
	ReadValue(info_dir + BBQUE_BATTERY_IF_CHARGE_MAH, mah, 9);
	if (!isdigit(mah[0])) {
		logger->Error("Not valid charge value");
		return 0;
	}
	return std::stol(mah) / 1000;
}

uint32_t Battery::GetVoltage() {
	char volt[6];
	ParseValue(status_dir + BBQUE_BATTERY_IF_PROC_STATE,
			BBQUE_BATTERY_PROC_STATE_VOLT, volt, 6);
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

	char rate[6];
	ParseValue(status_dir + BBQUE_BATTERY_IF_PROC_STATE,
			BBQUE_BATTERY_PROC_STATE_RATE, rate, 6);
	if (!isdigit(rate[0])) {
		logger->Error("Not valid discharging rate");
		return 0;
	}
	return std::stoi(rate);
}

unsigned long Battery::GetEstimatedLifetime() {
	float hours = (float) GetChargeMAh() / (float) GetDischargingRate();
	logger->Debug("Est. time: \thours=%.2f (min=%.2f)",
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


/*******************************************************************
 * Private member functions                                        *
 * *****************************************************************/

void Battery::ReadValue(std::string const & path, char * value, int len) {
	fd.open(path);
	fd.read(value, len);
	fd.close();
}

void Battery::ParseValue(
		std::string const & path,
		const char * pattern,
		char * value,
		int len) {
	std::string line;
	size_t b_pos, e_pos;

	fd.open(path);
	while (!fd.eof()) {
		std::getline(fd, line);
		// Pattern matching
		if (line.find(pattern) == std::string::npos)
			continue;
		// Value substring
		b_pos = line.find_first_of("0123456789");
		e_pos = line.find_last_of("0123456789") - b_pos;
		strncpy(value, (line.substr(b_pos, e_pos+1)).c_str(), len);
		break;
	}
	fd.close();
}

} // namespace bbque

