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

namespace bbque {


PowerMonitor & PowerMonitor::GetInstance() {
	static PowerMonitor instance;
	return instance;
}

PowerMonitor::PowerMonitor():
		Worker(),
		pm(PowerManager::GetInstance()),
		cm(CommandManager::GetInstance()) {
	// Get a logger module
	logger = bu::Logger::GetLogger(POWER_MONITOR_NAMESPACE);
	assert(logger);
	logger->Info("PowerMonitor initialization...");
	Init();

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

	return 0;
}

PowerMonitor::ExitCode_t PowerMonitor::Register(
		br::ResourcePathPtr_t rp,
		PowerManager::SamplesArray_t const & samples_window) {
	ResourceAccounter & ra(ResourceAccounter::GetInstance());

	// Register all the resources referenced by the path specified
	br::ResourcePtrList_t r_list(ra.GetResources(rp));
	if (r_list.empty()) {
		logger->Warn("No resources to monitor [%s]", rp->ToString().c_str());
		return ExitCode_t::ERR_RSRC_MISSING;
	}

	for (br::ResourcePtr_t rsrc: r_list) {
			// Number of samples for the exponential mean computation
			rsrc->EnablePowerProfile(samples_window);
			// The resource to monitor
			wm_info.resources.insert(
					std::pair<br::ResourcePathPtr_t, br::ResourcePtr_t>(
						ra.GetPath(rsrc->Path()), rsrc));
	}

	return ExitCode_t::OK;
}

void PowerMonitor::Start(uint32_t period_ms) {
	std::unique_lock<std::mutex> worker_status_ul(worker_status_mtx);
	if ((period_ms != 0) && (period_ms != wm_info.period_ms))
		wm_info.period_ms = period_ms;

	if (wm_info.started) {
		logger->Warn("Monitor already started with (T = %d ms)...",
			wm_info.period_ms);
		return;
	}

	logger->Info("Starting power monitoring (T = %d ms)...", wm_info.period_ms);
	events.set(WM_EVENT_UPDATE);
	worker_status_cv.notify_one();
}


void PowerMonitor::Stop() {
	std::unique_lock<std::mutex> worker_status_ul(worker_status_mtx);
	if (!wm_info.started) {
		logger->Warn("Monitor already stopped");
		return;
	}

	logger->Info("Stopping power monitoring...");
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

			std::stringstream ss_m;
			ss_m
				<< std::setprecision(str_p[info_idx]) << std::fixed
				<< std::setw(str_w[info_idx])         << std::left
				<< rsrc->GetPowerInfo(info_type, br::Resource::MEAN);
			log_mean_values += ss_m.str() + " ";
		}

		logger->Debug("Sampling: %s ", log_inst_values.c_str());
		logger->Debug("Sampling: %s ", log_mean_values.c_str());
	}

	return ExitCode_t::OK;
}

} // namespace bbque

