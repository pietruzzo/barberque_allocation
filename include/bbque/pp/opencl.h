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

#ifndef BBQUE_LINUX_OCLPROXY_H_
#define BBQUE_LINUX_OCLPROXY_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <CL/cl.h>

#include "bbque/modules_factory.h"
#include "bbque/app/application.h"
#include "bbque/res/identifier.h"

#define BBQUE_OCL_PLATFORM_NAME "Advanced Micro Devices, Inc."


using bbque::app::AppPtr_t;
using bbque::res::ResourceIdentifier;
using bbque::plugins::LoggerIF;


namespace bbque {

typedef std::vector<uint8_t> VectorUInt8_t;
typedef std::shared_ptr<VectorUInt8_t> VectorUInt8Ptr_t;

class OpenCLProxy {

public:

	enum ExitCode_t {
		SUCCESS,
		PLATFORM_ERROR,
		DEVICE_ERROR
	};

	/**
	 * @brief Constructor
	 */
	OpenCLProxy();

	/**
	 * @brief Load OpenCL platform data 
	 */
	ExitCode_t LoadPlatformData();

	/**
	 * @brief OpenCL resource assignment mapping
	 */
	ExitCode_t MapResources(AppPtr_t papp, UsagesMapPtr_t pum, RViewToken_t rvt);

	/**
	 * @brief Number of OpenCL devices of a given resource type
	 */
	uint8_t GetDevicesNum(ResourceIdentifier::Type_t r_type);

	/**
	 * @brief Set of OpenCL device IDs for a given resource type
	 */
	VectorUInt8Ptr_t GetDeviceIDs(ResourceIdentifier::Type_t r_type);
private:

	LoggerIF * logger;

	/** OpenCL platforms */
	cl_uint num_platforms;
	cl_platform_id * platforms;

	/** OpenCL devices */
	cl_uint num_devices;
	cl_device_id   * devices;

	/** Map with all the device IDs for each type available */
	std::map<ResourceIdentifier::Type_t, VectorUInt8Ptr_t> device_ids;

	/** Retrieve the iterator for the vector of device IDs, given a type */
	std::map<ResourceIdentifier::Type_t, VectorUInt8Ptr_t>::iterator
	GetDeviceIterator(ResourceIdentifier::Type_t r_type);

	/** Append a device ID in the map of all the IDs */
	void InsertDeviceID(ResourceIdentifier::Type_t r_type, uint8_t dev_id);

	/**
	 * @brief Register device resources
	 */
	ExitCode_t RegisterDevices();
};

} // namespace bbque

#endif
