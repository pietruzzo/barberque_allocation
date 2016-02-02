/*
 * Copyright (C) 2013  Politecnico di Milano
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

#include <cstring>

#include "bbque/pp/opencl.h"

#include "bbque/config.h"
#include "bbque/power_monitor.h"
#include "bbque/resource_accounter.h"
#include "bbque/res/binder.h"
#include "bbque/res/resource_path.h"
#include "bbque/app/working_mode.h"

#define MODULE_NAMESPACE "bq.pp.ocl"

namespace br = bbque::res;
namespace po = boost::program_options;

namespace bbque {

OpenCLProxy & OpenCLProxy::GetInstance() {
	static OpenCLProxy instance;
	return instance;
}

OpenCLProxy::OpenCLProxy():
		cm(ConfigurationManager::GetInstance()),
#ifndef CONFIG_BBQUE_PM
		cmm(CommandManager::GetInstance())
#else
		cmm(CommandManager::GetInstance()),
		pm(PowerManager::GetInstance())
#endif
 {
	//---------- Get a logger module
	logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	assert(logger);
}

OpenCLProxy::~OpenCLProxy() {
	delete platforms;
	delete devices;
	device_ids.clear();
	device_paths.clear();
}

OpenCLProxy::ExitCode_t OpenCLProxy::LoadPlatformData() {
	cl_int status;
	char platform_name[128];
	cl_platform_id platform = nullptr;
	cl_device_type dev_type = CL_DEVICE_TYPE_ALL;

	// Get platform
	status = clGetPlatformIDs(0, NULL, &num_platforms);
	if (status != CL_SUCCESS) {
		logger->Error("PLAT OCL: Platform error %d", status);
		return PLATFORM_ERROR;
	}
	logger->Info("PLAT OCL: Number of platform(s) found: %d", num_platforms);
	platforms = new cl_platform_id[num_platforms];
	status = clGetPlatformIDs(num_platforms, platforms, NULL);

	for (uint8_t i = 0; i < num_platforms; ++i) {
		status = clGetPlatformInfo(
			platforms[i], CL_PLATFORM_NAME,	sizeof(platform_name),
			platform_name, NULL);
		logger->Info("PLAT OCL: P[%d]: %s", i, platform_name);

		if (!strcmp(platform_name, BBQUE_OPENCL_PLATFORM)) {
			logger->Notice("PLAT OCL: Platform selected: %s", platform_name);
			platform = platforms[i];
			break;
		}
	}

	// Get devices
	status = clGetDeviceIDs(platform, dev_type, 0, NULL, &num_devices);
	if (status != CL_SUCCESS) {
		logger->Error("PLAT OCL: Device error %d", status);
		return DEVICE_ERROR;
	}
	logger->Info("PLAT OCL: Number of device(s) found: %d", num_devices);
	devices = new cl_device_id[num_devices];
	status  = clGetDeviceIDs(platform, dev_type, num_devices, devices, NULL);

	// Register into the resource accounter
	RegisterDevices();

	// Power management support
#ifdef CONFIG_BBQUE_PM
	PrintGPUPowerInfo();
#endif
	return SUCCESS;
}

void OpenCLProxy::Task() {

}

#ifdef CONFIG_BBQUE_PM
void OpenCLProxy::PrintGPUPowerInfo() {
	uint32_t min, max, step, s_min, s_max, ps_count;
	int  s_step;
	PowerManager::PMResult pm_result;

	ResourcePathListPtr_t pgpu_paths(
		GetDevicePaths(br::ResourceIdentifier::GPU));
	if (pgpu_paths == nullptr) return;

	for (auto gpu_rp: *(pgpu_paths.get())) {
		pm_result = pm.GetFanSpeedInfo(gpu_rp, min, max, step);
		if (pm_result == PowerManager::PMResult::OK)
			logger->Info("PLAT OCL: [%s] Fanspeed range: [%4d, %4d, s:%2d] RPM ",
				gpu_rp->ToString().c_str(), min, max, step);

		pm_result = pm.GetVoltageInfo(gpu_rp, min, max, step);
		if (pm_result == PowerManager::PMResult::OK)
			logger->Info("PLAT OCL: [%s] Voltage range:  [%4d, %4d, s:%2d] mV ",
				gpu_rp->ToString().c_str(), min, max, step);

		pm_result = pm.GetClockFrequencyInfo(gpu_rp, min, max, step);
		if (pm_result == PowerManager::PMResult::OK)
			logger->Info("PLAT OCL: [%s] ClkFreq range:  [%4d, %4d, s:%2d] MHz ",
				gpu_rp->ToString().c_str(),
				min/1000, max/1000, step/1000);

		std::vector<unsigned long> freqs;
		std::string freqs_str(" ");
		pm_result = pm.GetAvailableFrequencies(gpu_rp, freqs);
		if (pm_result == PowerManager::PMResult::OK) {
			for (auto & f: freqs)
				freqs_str += (std::to_string(f) + " ");
			logger->Info("PLAT OCL: [%s] ClkFrequencies:  [%s] MHz ",
					gpu_rp->ToString().c_str(), freqs_str.c_str());
		}

		pm_result = pm.GetPowerStatesInfo(gpu_rp, s_min,s_max, s_step);
		if (pm_result == PowerManager::PMResult::OK)
			logger->Info("PLAT OCL: [%s] Power states:   [%4d, %4d, s:%2d] ",
				gpu_rp->ToString().c_str(), s_min, s_max, s_step);

		pm_result = pm.GetPerformanceStatesCount(gpu_rp, ps_count);
		if (pm_result == PowerManager::PMResult::OK)
			logger->Info("PLAT OCL: [%s] Performance states: %2d",
				gpu_rp->ToString().c_str(), ps_count);
//		pm.SetFanSpeed(gpu_rp,PowerManager::FanSpeedType::PERCENT, 5);
//		pm.ResetFanSpeed(gpu_rp);
	}
	logger->Info("PLAT OCL: Monitoring %d GPU adapters", pgpu_paths->size());
}
#endif // CONFIG_BBQUE_PM


VectorUInt8Ptr_t OpenCLProxy::GetDeviceIDs(br::ResourceIdentifier::Type_t r_type) {
	ResourceTypeIDMap_t::iterator d_it;
	d_it = GetDeviceIterator(r_type);
	if (d_it == device_ids.end()) {
		logger->Error("PLAT OCL: No OpenCL devices of type '%s'",
			br::ResourceIdentifier::TypeStr[r_type]);
		return nullptr;
	}
	return d_it->second;
}


uint8_t OpenCLProxy::GetDevicesNum(br::ResourceIdentifier::Type_t r_type) {
	ResourceTypeIDMap_t::iterator d_it;
	d_it = GetDeviceIterator(r_type);
	if (d_it == device_ids.end()) {
		logger->Error("PLAT OCL: No OpenCL devices of type '%s'",
			br::ResourceIdentifier::TypeStr[r_type]);
		return 0;
	}
	return d_it->second->size();
}

ResourcePathListPtr_t OpenCLProxy::GetDevicePaths(br::ResourceIdentifier::Type_t r_type) {
	ResourceTypePathMap_t::iterator p_it;
	p_it = device_paths.find(r_type);
	if (p_it == device_paths.end()) {
		logger->Error("PLAT OCL: No OpenCL devices of type  '%s'",
			br::ResourceIdentifier::TypeStr[r_type]);
		return nullptr;
	}
	return p_it->second;
}


ResourceTypeIDMap_t::iterator
OpenCLProxy::GetDeviceIterator(br::ResourceIdentifier::Type_t r_type) {
	if (platforms == nullptr) {
		logger->Error("PLAT OCL: Missing OpenCL platforms");
		return device_ids.end();
	}
	return	device_ids.find(r_type);
}

OpenCLProxy::ExitCode_t OpenCLProxy::RegisterDevices() {
	cl_int status;
	cl_device_type dev_type;
	char dev_name[64];
	char gpu_pe_path[]  = "sys0.gpu256.pe256";
	br::ResourceIdentifier::Type_t r_type = br::ResourceIdentifier::UNDEFINED;
	ResourceAccounter &ra(ResourceAccounter::GetInstance());
#ifdef CONFIG_BBQUE_WM
	PowerMonitor & wm(PowerMonitor::GetInstance());
#endif
	for (uint16_t dev_id = 0; dev_id < num_devices; ++dev_id) {
		status = clGetDeviceInfo(
				devices[dev_id], CL_DEVICE_NAME,
				sizeof(dev_name), dev_name, NULL);
		status = clGetDeviceInfo(
				devices[dev_id], CL_DEVICE_TYPE,
				sizeof(dev_type), &dev_type, NULL);
		if (status != CL_SUCCESS) {
			logger->Error("PLAT OCL: Device info error %d", status);
			return DEVICE_ERROR;
		}

		if (dev_type != CL_DEVICE_TYPE_CPU &&
			dev_type != CL_DEVICE_TYPE_GPU) {
			logger->Warn("PLAT OCL: Unexpected device type [%d]",
				dev_type);
			continue;
		}

		switch (dev_type) {
		case CL_DEVICE_TYPE_GPU:
			snprintf(gpu_pe_path+5, 12, "gpu%hu.pe0", dev_id);
			ra.RegisterResource(gpu_pe_path, "", 100);
			r_type = br::ResourceIdentifier::GPU;
#ifdef CONFIG_BBQUE_WM
			wm.Register(ra.GetPath(gpu_pe_path));
#endif
			break;
		case CL_DEVICE_TYPE_CPU:
			r_type = br::ResourceIdentifier::CPU;
			memset(gpu_pe_path, '\0', strlen(gpu_pe_path));
			break;
		}

		// Keep track of OpenCL device IDs and resource paths
		InsertDeviceID(r_type, dev_id);
		if (r_type == br::ResourceIdentifier::GPU)
			InsertDevicePath(r_type, gpu_pe_path);

		logger->Info("PLAT OCL: D[%d]: {%s}, type: [%s], path: [%s]",
			dev_id, dev_name,
			br::ResourceIdentifier::TypeStr[r_type],
			gpu_pe_path);
	}

	return SUCCESS;
}


void OpenCLProxy::InsertDeviceID(
		br::ResourceIdentifier::Type_t r_type,
		uint8_t dev_id) {
	ResourceTypeIDMap_t::iterator d_it;
	VectorUInt8Ptr_t pdev_ids;

	logger->Debug("PLAT OCL: Insert device %d of type %s",
			dev_id, br::ResourceIdentifier::TypeStr[r_type]);
	d_it = GetDeviceIterator(r_type);
	if (d_it == device_ids.end()) {
		device_ids.insert(
			std::pair<br::ResourceIdentifier::Type_t, VectorUInt8Ptr_t>
				(r_type, VectorUInt8Ptr_t(new VectorUInt8_t))
		);
	}

	pdev_ids = device_ids[r_type];
	pdev_ids->push_back(dev_id);
	if (r_type != br::ResourceIdentifier::GPU)
		return;

	// Resource path to GPU memory
	char gpu_mem_path[] = "sys0.gpu256.mem256";
	snprintf(gpu_mem_path+5, 12, "gpu%hu.mem0", dev_id);
	gpu_mem_paths.insert(std::pair<int, ResourcePathPtr_t>(
		dev_id, ResourcePathPtr_t(new br::ResourcePath(gpu_mem_path))));
	logger->Debug("PLAT OCL: GPU memory registered: %s", gpu_mem_path);
}

void OpenCLProxy::InsertDevicePath(
		br::ResourceIdentifier::Type_t r_type,
		std::string const & dev_path_str) {
	ResourceTypePathMap_t::iterator p_it;
	ResourcePathListPtr_t pdev_paths;

	logger->Debug("PLAT OCL: Insert device resource path  %s",
			dev_path_str.c_str());
	p_it = device_paths.find(r_type);
	if (p_it == device_paths.end()) {
		device_paths.insert(
			std::pair<br::ResourceIdentifier::Type_t, ResourcePathListPtr_t>
				(r_type, ResourcePathListPtr_t(new ResourcePathList_t))
		);
	}

	ResourcePathPtr_t rp(new br::ResourcePath(dev_path_str));
	if (rp == nullptr) {
		logger->Error("PLAT OCL: Invalid resource path %s",
				dev_path_str.c_str());
		return;
	}

	pdev_paths = device_paths[r_type];
	pdev_paths->push_back(rp);
}

OpenCLProxy::ExitCode_t OpenCLProxy::MapResources(
		ba::AppPtr_t papp,
		br::UsagesMapPtr_t pum,
		br::RViewToken_t rvt) {
	(void)papp;
	(void)pum;
	(void)rvt;
	logger->Warn("PLAT OCL: Please map the application");

	return SUCCESS;
}

int OpenCLProxy::CommandsCb(int argc, char *argv[]) {
	(void) argc;
	uint8_t cmd_offset = ::strlen(MODULE_NAMESPACE) + 1;
	char * command_id  = argv[0] + cmd_offset;
	logger->Error("PLAT OCL: Unknown command [%s]", command_id);
	return -1;
}

} // namespace bbque
