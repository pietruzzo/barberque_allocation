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
#include "bbque/res/resource_path.h"
#include "bbque/app/working_mode.h"

#define MODULE_NAMESPACE "bq.pp.ocl"


using bbque::res::ResourceIdentifier;

namespace bbque {

OpenCLProxy & OpenCLProxy::GetInstance() {
	static OpenCLProxy instance;
	return instance;
}

OpenCLProxy::OpenCLProxy() {
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
	logger->Info("PLAT OCL: Number of platform(s) found: %d", num_platforms);
	platforms = new cl_platform_id[num_platforms];
	status = clGetPlatformIDs(num_platforms, platforms, NULL);

	for (uint8_t i = 0; i < num_platforms; ++i) {
		status = clGetPlatformInfo(
			platforms[i], CL_PLATFORM_NAME,	sizeof(platform_name),
			platform_name, NULL);
		logger->Info("PLAT OCL: P[%d]: %s", i, platform_name);

		if (!strcmp(platform_name, BBQUE_PLATFORM_NAME)) {
			logger->Info("PLAT OCL: Platform selected: %s", platform_name);
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

	return SUCCESS;
}


VectorUInt8Ptr_t OpenCLProxy::GetDeviceIDs(ResourceIdentifier::Type_t r_type) {
	ResourceTypeIDMap_t::iterator d_it;
	d_it = GetDeviceIterator(r_type);
	if (d_it == device_ids.end()) {
		logger->Error("PLAT OCL: No OpenCL devices of type '%s'",
			ResourceIdentifier::TypeStr[r_type]);
		return nullptr;
	}
	return d_it->second;
}


uint8_t OpenCLProxy::GetDevicesNum(ResourceIdentifier::Type_t r_type) {
	ResourceTypeIDMap_t::iterator d_it;
	d_it = GetDeviceIterator(r_type);
	if (d_it == device_ids.end()) {
		logger->Error("PLAT OCL: No OpenCL devices of type '%s'",
			ResourceIdentifier::TypeStr[r_type]);
		return 0;
	}
	return d_it->second->size();
}

ResourcePathListPtr_t OpenCLProxy::GetDevicePaths(ResourceIdentifier::Type_t r_type) {
	ResourceTypePathMap_t::iterator p_it;
	p_it = device_paths.find(r_type);
	if (p_it == device_paths.end()) {
		logger->Error("PLAT OCL: No OpenCL devices of type  '%s'",
			ResourceIdentifier::TypeStr[r_type]);
		return nullptr;
	}
	return p_it->second;
}


ResourceTypeIDMap_t::iterator
OpenCLProxy::GetDeviceIterator(ResourceIdentifier::Type_t r_type) {
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
	char resourcePath[] = "sys0.gpu256.pe256";
	ResourceIdentifier::Type_t r_type = ResourceIdentifier::UNDEFINED;
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
			snprintf(resourcePath+5, 12, "gpu%hu.pe0", dev_id);
			ra.RegisterResource(resourcePath, "", 100);
			r_type = ResourceIdentifier::GPU;
			break;
		case CL_DEVICE_TYPE_CPU:
			r_type = ResourceIdentifier::CPU;
			memset(resourcePath, '\0', strlen(resourcePath));
			break;
		}

		InsertDeviceID(r_type, dev_id);
		InsertDevicePath(r_type, resourcePath);
		logger->Info("PLAT OCL: D[%d]: {%s}, type: [%s], path: [%s]",
			dev_id, dev_name,
			ResourceIdentifier::TypeStr[r_type],
			resourcePath);
	}

	return SUCCESS;
}


void OpenCLProxy::InsertDeviceID(
		ResourceIdentifier::Type_t r_type,
		uint8_t dev_id) {
	ResourceTypeIDMap_t::iterator d_it;
	VectorUInt8Ptr_t pdev_ids;
	d_it = GetDeviceIterator(r_type);
	if (d_it == device_ids.end()) {
		device_ids.insert(
			std::pair<ResourceIdentifier::Type_t, VectorUInt8Ptr_t>
				(r_type, VectorUInt8Ptr_t(new VectorUInt8_t))
		);
	}

	pdev_ids = device_ids[r_type];
	pdev_ids->push_back(dev_id);
}

void OpenCLProxy::InsertDevicePath(
		ResourceIdentifier::Type_t r_type,
		std::string const & dev_path_str) {
	ResourceTypePathMap_t::iterator p_it;
	ResourcePathListPtr_t pdev_paths;
	p_it = device_paths.find(r_type);
	if (p_it == device_paths.end()) {
		device_paths.insert(
			std::pair<ResourceIdentifier::Type_t, ResourcePathListPtr_t>
				(r_type, ResourcePathListPtr_t(new ResourcePathList_t))
		);
	}

	ResourcePathPtr_t rp(new ResourcePath(dev_path_str));
	pdev_paths = device_paths[r_type];
	pdev_paths->push_back(rp);
}

OpenCLProxy::ExitCode_t OpenCLProxy::MapResources(
		AppPtr_t papp,
		UsagesMapPtr_t pum,
		RViewToken_t rvt) {
	logger->Warn("PLAT OCL: Please map the application");

	return SUCCESS;
}

} // namespace bbque
