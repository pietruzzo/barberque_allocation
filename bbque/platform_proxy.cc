/*
 * Copyright (C) 2012  Politecnico di Milano
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
#include <string>
#include <sstream>

#include "bbque/platform_proxy.h"
#include "bbque/modules_factory.h"
#include "bbque/resource_manager.h"
#include "bbque/res/resource_path.h"
#include "bbque/utils/utility.h"

#ifdef CONFIG_BBQUE_TEST_PLATFORM_DATA
# warning Using Test Platform Data (TPD)
# define PLATFORM_PROXY PlatformProxy // Use the base class when TPD in use
#else // CONFIG_BBQUE_TEST_PLATFORM_DATA
# ifdef CONFIG_TARGET_LINUX
#  include "bbque/pp/linux.h"
#  define  PLATFORM_PROXY LinuxPP
# endif
# ifdef CONFIG_TARGET_P2012
#  include "bbque/pp/p2012.h"
#  define  PLATFORM_PROXY P2012PP
# endif
#endif // CONFIG_BBQUE_TEST_PLATFORM_DATA

#define PLATFORM_PROXY_NAMESPACE "bq.pp"
#define MODULE_NAMESPACE PLATFORM_PROXY_NAMESPACE

namespace br = bbque::res;

namespace bbque {

PlatformProxy::PlatformProxy() : Worker(),
	pilInitialized(false),
#ifndef CONFIG_BBQUE_PM
	platformIdentifier(NULL) {
#else
	platformIdentifier(NULL),
	pm(PowerManager::GetInstance()) {

	InitPowerMonitor();
#endif
	//---------- Setup Worker
	Worker::Setup(BBQUE_MODULE_NAME("pp"), PLATFORM_PROXY_NAMESPACE);

#ifdef CONFIG_BBQUE_TEST_PLATFORM_DATA
	// Mark the Platform Integration Layer (PIL) as initialized
	SetPilInitialized();
#endif // !CONFIG_BBQUE_TEST_PLATFORM_DATA

}

PlatformProxy::~PlatformProxy() {
}

PlatformProxy & PlatformProxy::GetInstance() {
	static PLATFORM_PROXY instance;
	return instance;
}


void PlatformProxy::Refresh() {
	std::unique_lock<std::mutex> worker_status_ul(worker_status_mtx);
	// Notify the platform monitoring thread about a new event ot be
	// processed
	platformEvents.set(PLATFORM_EVT_REFRESH);
	worker_status_cv.notify_one();
}

void PlatformProxy::Task() {
#ifndef CONFIG_BBQUE_TEST_PLATFORM_DATA

	logger->Info("PLAT PRX: Monitoring thread STARTED");

	while (1) {
		if (platformEvents.none()) {
			logger->Info("PLAT PRX: No platform event to process");
			Wait();
		}

		// Refresh available resources
		if (platformEvents.test(PLATFORM_EVT_REFRESH))
			RefreshPlatformData();

#ifdef CONFIG_BBQUE_PM
		// Monitor the power thermal status
		if (platformEvents.test(PLATFORM_EVT_MONITOR)) {
			PowerMonitorSample();
			std::this_thread::sleep_for(
					std::chrono::milliseconds(powerMonitorInfo.period_ms));
		}
#endif // CONFIG_BBQUE_PM
	}

	logger->Info("PLAT PRX: Monitoring thread ENDED");
#else
	logger->Info("PLAT PRX: Terminating monitoring thread (TPD in use)");
	return;
#endif // CONFIG_BBQUE_TEST_PLATFORM_DATA

}


PlatformProxy::ExitCode_t
PlatformProxy::LoadPlatformData() {
	ResourceAccounter &ra(ResourceAccounter::GetInstance());
	ExitCode_t result = OK;

	// Return if the PIL has not been properly initialized
	if (!pilInitialized) {
		logger->Fatal("PLAT PRX: Platform Integration Layer initialization FAILED");
		return PLATFORM_INIT_FAILED;
	}

	// Platform specific resources enumeration
	logger->Debug("PLAT PRX: loading platform data");
	result = _LoadPlatformData();
	if (unlikely(result != OK)) {
		logger->Fatal("PLAT PRX: Platform [%s] initialization FAILED",
				GetPlatformID());
		return result;
	}

	// Setup the Platform Specific ID
	platformIdentifier = _GetPlatformID();
	hardwareIdentifier = _GetHardwareID();

	logger->Notice("PLAT PRX: Platform [%s] initialization COMPLETED",
			GetPlatformID());

	// Set that the platform is ready
	ra.SetPlatformReady();

	// Dump status of registered resource
	ra.PrintStatusReport(0, true);

	return result;
}

PlatformProxy::ExitCode_t
PlatformProxy::RefreshPlatformData() {
	ResourceAccounter &ra(ResourceAccounter::GetInstance());
	ExitCode_t result = OK;

	// Set that the platform is NOT ready
	ra.SetPlatformNotReady();

	logger->Debug("PLAT PRX: refreshing platform description...");
	result = _RefreshPlatformData();
	if (result != OK) {
		ra.SetPlatformReady();
		return result;
	}

	CommitRefresh();

	return result;
}

PlatformProxy::ExitCode_t
PlatformProxy::CommitRefresh() {
	ResourceAccounter &ra(ResourceAccounter::GetInstance());
	ResourceManager &rm = ResourceManager::GetInstance();

	// TODO add a better policy which triggers immediate rescheduling only
	// on resources reduction. Perhaps such a policy could be plugged into
	// the ResourceManager module.

	// Set that the platform is ready
	platformEvents.reset(PLATFORM_EVT_REFRESH);
	ra.SetPlatformReady();

	// Notify a scheduling event to the ResourceManager
	rm.NotifyEvent(ResourceManager::BBQ_PLAT);

	return OK;
}


#ifdef CONFIG_BBQUE_PM

void PlatformProxy::InitPowerMonitor() {
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

PlatformProxy::ExitCode_t
PlatformProxy::PowerMonitorRegister(
		br::ResourcePathPtr_t rp,
		PowerManager::SamplesArray_t const & samples_window) {
	ResourceAccounter & ra(ResourceAccounter::GetInstance());

	// Register all the resources referenced by the path specified
	br::ResourcePtrList_t r_list(ra.GetResources(rp));
	if (r_list.empty()) {
		logger->Warn("No resources to monitor [%s]", rp->ToString().c_str());
		return PLATFORM_PWR_MONITOR_ERROR;
	}

	// Specifiy the number of samples for the exponential mean computation
	for (br::ResourcePtr_t rsrc: r_list) {
			rsrc->EnablePowerProfile(samples_window);
			powerMonitorInfo.resources.insert(
					std::pair<br::ResourcePathPtr_t, br::ResourcePtr_t>(
						ra.GetPath(rsrc->Path()), rsrc));
	}

	return PlatformProxy::OK;
}

void PlatformProxy::PowerMonitorStart(uint32_t period_ms) {
	std::unique_lock<std::mutex> worker_status_ul(worker_status_mtx);
	if (period_ms != 0)
		powerMonitorInfo.period_ms = period_ms;
	logger->Info("Starting power monitoring (T = %d ms)...",
			powerMonitorInfo.period_ms);
	platformEvents.set(PLATFORM_EVT_MONITOR);
	worker_status_cv.notify_one();
}

void PlatformProxy::PowerMonitorStop() {
	logger->Info("Stopping power monitoring...");
	std::unique_lock<std::mutex> worker_status_ul(worker_status_mtx);
	platformEvents.reset(PLATFORM_EVT_MONITOR);
}

PlatformProxy::ExitCode_t
PlatformProxy::PowerMonitorSample() {
	PowerManager::SamplesArray_t samples;
	PowerManager::InfoType info_type;

	// Power status monitoring over all the registered resources
	for (auto & r_entry: powerMonitorInfo.resources) {
		br::ResourcePathPtr_t const & r_path(r_entry.first);
		br::ResourcePtr_t rsrc(r_entry.second);

		std::string log_inst_values("Monitoring [");
		std::string log_mean_values("Monitoring [");
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

		logger->Debug("PLAT PRX: %s ", log_inst_values.c_str());
		logger->Debug("PLAT PRX: %s ", log_mean_values.c_str());
	}

	return PlatformProxy::OK;
}

#endif // CONFIG_BBQUE_PM


PlatformProxy::ExitCode_t
PlatformProxy::Setup(AppPtr_t papp) {
	ExitCode_t result = OK;

	logger->Debug("PLAT PRX: platform setup for run-time control "
			"of app [%s]", papp->StrId());
	result = _Setup(papp);
	return result;
}

PlatformProxy::ExitCode_t
PlatformProxy::Release(AppPtr_t papp) {
	ExitCode_t result = OK;

	logger->Debug("PLAT PRX: releasing platform-specific run-time control "
			"for app [%s]", papp->StrId());
	result = _Release(papp);
	return result;
}

PlatformProxy::ExitCode_t
PlatformProxy::ReclaimResources(AppPtr_t papp) {
	ExitCode_t result = OK;

	logger->Debug("PLAT PRX: Reclaiming resources of app [%s]", papp->StrId());
	result = _ReclaimResources(papp);
	return result;
}

PlatformProxy::ExitCode_t
PlatformProxy::MapResources(AppPtr_t papp, UsagesMapPtr_t pres, bool excl) {
	ResourceAccounter &ra = ResourceAccounter::GetInstance();
	RViewToken_t rvt = ra.GetScheduledView();
	ExitCode_t result = OK;

	logger->Debug("PLAT PRX: Mapping resources for app [%s], using view [%d]",
			papp->StrId(), rvt);

	// Platform Specific Data (PSD) should be initialized the first time
	// an application is scheduled for execution
	if (unlikely(!papp->HasPlatformData())) {

		// Setup PSD
		result = Setup(papp);
		if (result != OK) {
			logger->Error("Setup PSD for EXC [%s] FAILED",
					papp->StrId());
			return result;
		}

		// Mark PSD as correctly initialized
		papp->SetPlatformData();
	}

	// Map resources
	result = _MapResources(papp, pres, rvt, excl);

	return result;
}

} /* bbque */
