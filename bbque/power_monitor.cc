/*
 * Copyright (C) 2014  Politecnico di Milano
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

#include <iomanip>
#include <sstream>
#include <string>

#include "bbque/power_monitor.h"

#include "bbque/res/resource_path.h"

#define MODULE_CONFIG "PowerMonitor"
#define MODULE_NAMESPACE POWER_MONITOR_NAMESPACE

#define WM_LOGFILE_FMT "%s/BBQ_PowerMonitor_%.dat"
#define WM_LOGFILE_HEADER \
	"# Columns legend:\n"\
	"#\n"\
	"# 1: Load (%)\n"\
	"# 2: Temperature (°C)\n"\
	"# 3: Core frequency (MHz)\n"\
	"# 4: Fanspeed (%)\n"\
	"# 5: Voltage (mV)\n"\
	"# 6: Performance level\n"\
	"# 7: Power state\n"\
	"#\n"


namespace po = boost::program_options;

namespace bbque {


PowerMonitor & PowerMonitor::GetInstance() {
	static PowerMonitor instance;
	return instance;
}

PowerMonitor::PowerMonitor():
		Worker(),
		pm(PowerManager::GetInstance()),
		cm(CommandManager::GetInstance()),
		cfm(ConfigurationManager::GetInstance()) {
	// Get a logger module
	logger = bu::Logger::GetLogger(POWER_MONITOR_NAMESPACE);
	assert(logger);
	logger->Info("PowerMonitor initialization...");
	Init();

	//---------- Loading configuration
	po::options_description opts_desc("Power Monitor options");
	opts_desc.add_options()
		(MODULE_CONFIG ".period_ms",
		 po::value<uint32_t>(&wm_info.period_ms)->default_value(WM_DEFAULT_PERIOD_MS),
		 "The period [ms] power monitor sampling");

	opts_desc.add_options()
		(MODULE_CONFIG ".log.dir",
		 po::value<std::string>(&wm_info.log_dir)->default_value("/tmp/"),
		 "The output directory for the status data dump files");
	po::variables_map opts_vm;
	cfm.ParseConfigurationFile(opts_desc, opts_vm);

#define CMD_WM_DATALOG "datalog"
	cm.RegisterCommand(MODULE_NAMESPACE "." CMD_WM_DATALOG,
			static_cast<CommandHandler*>(this),
			"Start/stop power monitor data logging");

	//---------- Setup Worker
	Worker::Setup(BBQUE_MODULE_NAME("wm"), POWER_MONITOR_NAMESPACE);
	Worker::Start();
}

void PowerMonitor::Init() {
	PowerMonitorGet[size_t(PowerManager::InfoType::LOAD)]        =
		&PowerManager::GetLoad;
	PowerMonitorGet[size_t(PowerManager::InfoType::TEMPERATURE)] =
		&PowerManager::GetTemperature;
	PowerMonitorGet[size_t(PowerManager::InfoType::FREQUENCY)]   =
		&PowerManager::GetClockFrequency;
	PowerMonitorGet[size_t(PowerManager::InfoType::POWER)]       =
		&PowerManager::GetPowerUsage;
	PowerMonitorGet[size_t(PowerManager::InfoType::PERF_STATE)]  =
		&PowerManager::GetPerformanceState;
	PowerMonitorGet[size_t(PowerManager::InfoType::POWER_STATE)] =
		&PowerManager::GetPowerState;
}

PowerMonitor::~PowerMonitor() {

}

void PowerMonitor::Task() {
	while (1) {
		if (events.none()) {
			logger->Info("No events to process");
			Wait();
		}

		// Monitor the power thermal status
		if (events.test(WM_EVENT_UPDATE)) {
			Sample();
			std::this_thread::sleep_for(
					std::chrono::milliseconds(wm_info.period_ms));
		}
	}
}

int PowerMonitor::CommandsCb(int argc, char *argv[]) {
	uint8_t cmd_offset = ::strlen(MODULE_NAMESPACE) + 1;
	char * command_id  = argv[0] + cmd_offset;
	logger->Info("PWR MNTR: Processing command [%s]", command_id);

	// Data logging control
	if (!strncmp(CMD_WM_DATALOG, command_id, strlen(CMD_WM_DATALOG))) {
		if (argc != 2) {
			logger->Error("PWR MNTR: Command [%s] missing action [start/stop/clear]",
					command_id);
			return 1;
		}
		return DataLogCmdHandler(argv[1]);
	}

	logger->Error("PWR MNTR: Command unknown [%s]", command_id);
	return -1;
}


PowerMonitor::ExitCode_t PowerMonitor::Register(
		br::ResourcePathPtr_t rp,
		PowerManager::SamplesArray_t const & samples_window) {
	ResourceAccounter & ra(ResourceAccounter::GetInstance());

	// Register all the resources referenced by the path specified
	br::ResourcePtrList_t r_list(ra.GetResources(rp));
	if (r_list.empty()) {
		logger->Warn("PWR MNTR: No resources to monitor [%s]", rp->ToString().c_str());
		return ExitCode_t::ERR_RSRC_MISSING;
	}

	for (br::ResourcePtr_t rsrc: r_list) {
			// Number of samples for the exponential mean computation
			rsrc->EnablePowerProfile(samples_window);
			// The resource to monitor
			wm_info.resources.insert(
					std::pair<br::ResourcePathPtr_t, br::ResourcePtr_t>(
						ra.GetPath(rsrc->Path()), rsrc));
			logger->Info("PWR MNTR: Registering [%s]...", rsrc->Path().c_str());
			// Resource data log file descriptor
			wm_info.log_fp.insert(
					std::pair<br::ResourcePathPtr_t, std::ofstream *>(
						ra.GetPath(rsrc->Path()), new std::ofstream()));
	}

	return ExitCode_t::OK;
}

PowerMonitor::ExitCode_t PowerMonitor::Register(
		const char * rp_str,
		PowerManager::SamplesArray_t const & samples_window) {
	ResourceAccounter & ra(ResourceAccounter::GetInstance());
	return Register(ra.GetPath(rp_str), samples_window);
}


void PowerMonitor::Start(uint32_t period_ms) {
	std::unique_lock<std::mutex> worker_status_ul(worker_status_mtx);
	if ((period_ms != 0) && (period_ms != wm_info.period_ms))
		wm_info.period_ms = period_ms;

	if (wm_info.started) {
		logger->Warn("PWR MNTR: Already started (T = %d ms)...",
			wm_info.period_ms);
		return;
	}

	logger->Info("PWR MNTR: Starting (T = %d ms)...", wm_info.period_ms);
	events.set(WM_EVENT_UPDATE);
	worker_status_cv.notify_one();
}


void PowerMonitor::Stop() {
	std::unique_lock<std::mutex> worker_status_ul(worker_status_mtx);
	if (!wm_info.started) {
		logger->Warn("PWR MNTR: Already stopped");
		return;
	}

	logger->Info("PWR MNTR: Stopping...");
	events.reset(WM_EVENT_UPDATE);
}


PowerMonitor::ExitCode_t PowerMonitor::Sample() {
	PowerManager::SamplesArray_t samples;
	PowerManager::InfoType info_type;

	// Power status monitoring over all the registered resources
	for (auto & r_entry: wm_info.resources) {
		br::ResourcePathPtr_t const & r_path(r_entry.first);
		br::ResourcePtr_t rsrc(r_entry.second);

		std::string log_inst_values("[");
		std::string log_mean_values("[");
		std::string log_file_values;
		log_inst_values += rsrc->Path() + "] (I): ";
		log_mean_values += rsrc->Path() + "] (M): ";
		uint info_idx   = 0;
		uint info_count = 0;

		for (; info_idx < PowerManager::InfoTypeIndex.size() &&
				info_count < rsrc->GetPowerInfoEnabledCount();
					++info_idx, ++info_count) {
			// Check if the power profile information has been required
			info_type = PowerManager::InfoTypeIndex[info_idx];
			if (rsrc->GetPowerInfoSamplesWindowSize(info_type) <= 0)
				continue;

			// Call power manager get function and update the resource
			// descriptor power profile information
			(pm.*(PMfunc) PowerMonitorGet[info_idx])(r_path, samples[info_idx]);
			rsrc->UpdatePowerInfo(info_type, samples[info_idx]);

			// Log messages
			std::stringstream ss_i;
			ss_i
				<< std::setprecision(0)       << std::fixed
				<< std::setw(str_w[info_idx]) << std::left
				<< rsrc->GetPowerInfo(info_type, br::Resource::INSTANT);
			log_inst_values += ss_i.str() + " ";
			log_file_values += ss_i.str() + " ";

			std::stringstream ss_m;
			ss_m
				<< std::setprecision(str_p[info_idx]) << std::fixed
				<< std::setw(str_w[info_idx])         << std::left
				<< rsrc->GetPowerInfo(info_type, br::Resource::MEAN);
			log_mean_values += ss_m.str() + " ";
		}

		logger->Debug("PWR MNTR: Sampling [%s] ", log_inst_values.c_str());
		logger->Debug("PWR MNTR: Sampling [%s] ", log_mean_values.c_str());
		if (wm_info.log_enabled) {
			DataLogWrite(r_path, log_file_values);
		}
	}

	return ExitCode_t::OK;
}

/*******************************************************************
 *                        DATA LOGGING                             *
 *******************************************************************/

void PowerMonitor::DataLogWrite(
		br::ResourcePathPtr_t rp,
		std::string const & data_line,
		std::ios_base::openmode om) {

	//std::string file_path(BBQUE_WM_DATALOG_PATH "/");
	std::string file_path(wm_info.log_dir + "/");
	file_path.append(rp->ToString());
	file_path.append(".dat");
	logger->Debug("PWR MNTR: Log file [%s]: %s",
		file_path.c_str(), data_line.c_str());

	// Open file
	wm_info.log_fp[rp]->open(file_path, om);
	if (!wm_info.log_fp[rp]->is_open()) {
		logger->Warn("PWR MNTR: Log file not open");
		return;
	}

	// Write data line
	*wm_info.log_fp[rp] << data_line << std::endl;
	if (wm_info.log_fp[rp]->fail()) {
		logger->Error("PWR MNTR: Log file write failed [F:%d, B:%d]",
			wm_info.log_fp[rp]->fail(),
			wm_info.log_fp[rp]->bad());
		*wm_info.log_fp[rp] << "Error";
		*wm_info.log_fp[rp] << std::endl;
		return;
	}
	// Close file
	wm_info.log_fp[rp]->close();
}


void PowerMonitor::DataLogClear() {
	for (auto log_ofs: wm_info.log_fp) {
		DataLogWrite(log_ofs.first, WM_LOGFILE_HEADER, std::ios_base::out);
	}
}


int PowerMonitor::DataLogCmdHandler(const char * arg) {
	std::string action(arg);
	logger->Info("PWR MNTR: Action = %s", action.c_str());
	// Start
	if ((action.compare("start") == 0)
			&& (!wm_info.log_enabled)) {
		logger->Info("PWR MNTR: Starting data logging...");
		wm_info.log_enabled = true;
		return 0;
	}
	// Stop
	if ((action.compare("stop") == 0)
			&& (wm_info.log_enabled)) {
		logger->Info("PWR MNTR: Stopping data logging...");
		wm_info.log_enabled = false;
		return 0;
	}
	// Clear
	if (action.compare("clear") == 0) {
		bool de = wm_info.log_enabled;
		wm_info.log_enabled = false;
		DataLogClear();
		wm_info.log_enabled = de;
		logger->Info("PWR MNTR: Clearing data logs...");
		return 0;
	}

	logger->Warn("PWR MNTR: unknown action [%s] or nothing to do",
		action.c_str());
	return -1;
}

} // namespace bbque

