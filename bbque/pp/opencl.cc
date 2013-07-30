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

#include "bbque/resource_accounter.h"
#include "bbque/res/binder.h"
#include "bbque/app/working_mode.h"

#define MODULE_NAMESPACE "bq.pp.ocl"

namespace bbque {


OpenCLProxy::OpenCLProxy():
	num_platforms(0),
	platforms(nullptr),
	num_devices(0),
	devices(nullptr) {

	// Get a logger
	LoggerIF::Configuration conf(MODULE_NAMESPACE);
	logger = ModulesFactory::GetLoggerModule(std::cref(conf));
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
	logger->Info("PLAT OCL: %d platform(s) found", num_platforms);
	platforms = new cl_platform_id[num_platforms];
	status = clGetPlatformIDs(num_platforms, platforms, NULL);

	for (uint8_t i = 0; i < num_platforms; ++i) {
		status = clGetPlatformInfo(
				platforms[i], CL_PLATFORM_VENDOR,
				sizeof(platform_name), platform_name, NULL);

		if (!strcmp(platform_name, BBQUE_OCL_PLATFORM_NAME)) {
				platform = platforms[i];
				break;
		}
	}

	// Get devices
	status = clGetDeviceIDs(platform, dev_type, 0, NULL, &num_devices);
	if (status != CL_SUCCESS)
	{
		logger->Error("PLAT OCL: Device error %d", status);
		return DEVICE_ERROR;
	}
	logger->Info("PLAT OCL: %d device(s) found", num_devices);
	devices = new cl_device_id[num_devices];
	status  = clGetDeviceIDs(platform, dev_type, num_devices, devices, NULL);

	// Register into the resource accounter
	RegisterDevices();

	return SUCCESS;
}


OpenCLProxy::ExitCode_t OpenCLProxy::RegisterDevices() {
	cl_int status;
	cl_device_type dev_type;
	char resourcePath[] = "sys0.gpu256.pe256";
	ResourceAccounter &ra(ResourceAccounter::GetInstance());

	for (uint8_t i = 0; i < num_devices; ++i) {
		status = clGetDeviceInfo(
				devices[i],	CL_DEVICE_TYPE,
				sizeof(dev_type), &dev_type, NULL);
		if (status != CL_SUCCESS) {
			logger->Error("PLAT OCL: Device info error %d", status);
			return DEVICE_ERROR;
		}
		logger->Info("PLAT OCL: Device type %d", dev_type);
		
		switch (dev_type) {
		case CL_DEVICE_TYPE_GPU:
			// TODO: resource path
			snprintf(resourcePath+5, 12, "gpu%hu.pe0", i);
			ra.RegisterResource(resourcePath, "", 100);
			break;

		default:
			break;
		}
	}

	return SUCCESS;
}


OpenCLProxy::ExitCode_t OpenCLProxy::MapResources(
		AppPtr_t papp,
		UsagesMapPtr_t pum,
		RViewToken_t rvt) {
	logger->Warn("PLAT OCL: Please map the application");

	return SUCCESS;
}

} // namespace bbque
