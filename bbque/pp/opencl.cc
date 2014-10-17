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
#include "bbque/resource_accounter.h"
#include "bbque/res/binder.h"
#include "bbque/res/resource_path.h"
#include "bbque/app/working_mode.h"

#define MODULE_NAMESPACE "bq.pp.ocl"
#define MODULE_CONFIG    "PlatformProxy.OpenCL"

#define HWS_DUMP_FILE_FMT "%s/BBQ-PlatformStatus-GPU%d.dat"

#define HWS_DUMP_HEADER \
	"# Columns legend:\n"\
	"#\n"\
	"# 1: Adapter (GPU) ID \n"\
	"# 2: Load (%)\n"\
	"# 3: Temperature (°C)\n"\
	"# 4: Core frequency (MHz)\n"\
	"# 5: Mem frequency (MHz)\n"\
	"# 6: Fanspeed (%)\n"\
	"# 7: Voltage (mV)\n"\
	"# 8: Performance level\n"\
	"# 9: Power state\n"\
	"#\n"

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
#ifdef CONFIG_BBQUE_PM
	//---------- Loading configuration
	po::options_description opts_desc("Resource Manager Options");
	opts_desc.add_options()
		(MODULE_CONFIG ".hw.monitor_period_ms",
		 po::value<int32_t>
		 (&hw_monitor.period_ms)->default_value(-1),
		 "The period [ms] of activation of the periodic platform"
			" status reading");
	opts_desc.add_options()
		(MODULE_CONFIG ".hw.monitor_dump_dir",
		 po::value<std::string>
		 (&hw_monitor.dump_dir)->default_value(""),
		 "The output directory for the status data dump files");
	po::variables_map opts_vm;
	cm.ParseConfigurationFile(opts_desc, opts_vm);
	// Enable HW status dump?
	hw_monitor.dump_enabled = hw_monitor.dump_dir.compare("") != 0;

	// Register a command dispatcher to handle CGroups reconfiguration
#define CMD_PM_DUMP "pm_dump"
	cmm.RegisterCommand(MODULE_NAMESPACE "." CMD_PM_DUMP, static_cast<CommandHandler*>(this),
			"Start/stop dumping on file of GPU(s) power/thermal status");
#endif
}

OpenCLProxy::~OpenCLProxy() {
	delete platforms;
	delete devices;
	device_ids.clear();
	device_paths.clear();
#ifdef CONFIG_BBQUE_PM
	device_data.clear();
#endif
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
	HwSetup();
	Start();
#endif
	return SUCCESS;
}

void OpenCLProxy::Task() {
#ifdef CONFIG_BBQUE_PM
	if (hw_monitor.period_ms < 0)
		return;
	HwReadStatus();
#endif
}

#ifdef CONFIG_BBQUE_PM
void OpenCLProxy::HwSetup() {
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

void OpenCLProxy::HwReadStatus() {
	HWStatus_t hs;
	char status_line[100];

	if (device_paths.empty()) {
		logger->Warn("PLAT OCL: No resource path of devices found");
		return;
	}
	ResourcePathListPtr_t const & pgpu_paths(
		device_paths[br::ResourceIdentifier::GPU]);

	logger->Debug("PLAT OCL: Start monitoring [t=%d ms]...",
		hw_monitor.period_ms);
	while(1) {
		for (auto grp: *(pgpu_paths.get())) {
			// Adapter ID
			hs.id = grp->GetID(br::ResourceIdentifier::GPU);
			// GPU status
			pm.GetLoad(grp, hs.load);
			pm.GetTemperature(grp, hs.temp);
			pm.GetClockFrequency(grp, hs.freq_c);
			pm.GetClockFrequency(gpu_mem_paths[hs.id], hs.freq_m);
			pm.GetFanSpeed(
				grp, PowerManager::FanSpeedType::PERCENT,
				hs.fan);
			pm.GetVoltage(grp, hs.mvolt);
			pm.GetPerformanceState(grp, hs.pstate);
			pm.GetPowerState(grp, hs.wstate);
			logger->Debug("PLAT PRX: GPU [%s] "
				"Load: %3d%, Temp: %3d°C, CoreFreq: %4dMHz, MemFreq: %4dMHz "
				"Fan: %3d%, Volt: %4dmV, PState: %2d, WState: %d",
				grp->ToString().c_str(),
				hs.load, hs.temp, hs.freq_c/1000, hs.freq_m/1000,
				hs.fan, hs.mvolt, hs.pstate, hs.wstate);
			// Dump status?
			if (!hw_monitor.dump_enabled)
				break;
			snprintf(status_line, 100,
				"%d %d %d %d %d %d %d %d %d\n",
				hs.id, hs.load, hs.temp,
				hs.freq_c/1000, hs.freq_m/1000,	hs.fan, hs.mvolt,
				hs.pstate, hs.wstate);
			DumpToFile(hs.id, status_line, std::ios::app);
		}
		std::this_thread::sleep_for(
			std::chrono::milliseconds(hw_monitor.period_ms));
	}
}

void OpenCLProxy::DumpToFile(
		int dev_id, const char * line, std::ios_base::openmode om) {
	char fp[128];
	snprintf(fp, 128, HWS_DUMP_FILE_FMT, hw_monitor.dump_dir.c_str(), dev_id);
	logger->Debug("Dump > [%s]: %s", fp, line);

	device_data[dev_id]->open(fp, om);
	if (!device_data[dev_id]->is_open()) {
		logger->Warn("PLAT OCL: Dump file not open");
		return;
	}
	*device_data[dev_id] << line;
	if (device_data[dev_id]->fail()) {
		logger->Error("PLAT OCL: Dump failed [F:%d, B:%d]",
			device_data[dev_id]->fail(),
			device_data[dev_id]->bad());
		*device_data[dev_id] << "Error";
		*device_data[dev_id] << std::endl;
		return;
	}
	device_data[dev_id]->close();
}

void OpenCLProxy::DumpClear() {
	for (auto dev_ofs: device_data) {
		DumpToFile(dev_ofs.first, HWS_DUMP_HEADER);
	}
}

int OpenCLProxy::DumpCmdHandler(const char * arg) {
	std::string action(arg);
	logger->Info("PLAT OCL: Action = %s", action.c_str());
	// Start
	if ((action.compare("start") == 0)
			&& (!hw_monitor.dump_enabled)) {
		logger->Info("PLAT OCL: Starting GPU(s) status dump...");
		hw_monitor.dump_enabled = true;
		return 0;
	}
	// Stop
	if ((action.compare("stop") == 0)
			&& (hw_monitor.dump_enabled)) {
		logger->Info("PLAT OCL: Stopping GPU(s) status dump...");
		hw_monitor.dump_enabled = false;
		return 0;
	}
	// Clear
	if (action.compare("clear") == 0) {
		bool de = hw_monitor.dump_enabled;
		hw_monitor.dump_enabled = false;
		DumpClear();
		hw_monitor.dump_enabled = de;
		logger->Info("PLAT OCL: Clearing GPU(s) status dump files...");
		return 0;
	}

	logger->Warn("PLAT OCL: Unknown action [%s] or nothing to do",
		action.c_str());
	return -1;
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
#ifdef CONFIG_BBQUE_PM
			device_data.insert(
				std::pair<int, std::ofstream *>(
					dev_id,
					new std::ofstream()));
			DumpToFile(dev_id, HWS_DUMP_HEADER );
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
	uint8_t cmd_offset = ::strlen(MODULE_NAMESPACE) + 1;
	char * command_id  = argv[0] + cmd_offset;
	logger->Info("PLAT OCL: Processing command [%s]", command_id);

#ifdef CONFIG_BBQUE_PM
	// GPU status dump start/stop
	if (!strncmp(CMD_PM_DUMP, command_id, strlen(CMD_PM_DUMP))) {
		if (argc != 2) {
			logger->Error("PLAT OCL: Command [%s]: "
				"missing action [start/stop/clear]", command_id);
			return 1;
		}
		return DumpCmdHandler(argv[1]);
	}
#endif
	logger->Error("PLAT OCL: Unknown command [%s]", command_id);
	return -1;
}

} // namespace bbque
