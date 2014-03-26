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

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <CL/cl.h>

#include "bbque/config.h"
#include "bbque/modules_factory.h"
#include "bbque/app/application.h"
#include "bbque/pm/power_manager.h"
#include "bbque/res/identifier.h"
#include "bbque/utils/worker.h"

#define AMD_PLATFORM_NAME   "AMD Accelerated Parallel Processing"
#define INTEL_PLATFORM_NAME "Intel(R) OpenCL"
#define BBQUE_PLATFORM_NAME AMD_PLATFORM_NAME


using bbque::app::AppPtr_t;
using bbque::res::ResourceIdentifier;
using bbque::plugins::LoggerIF;


namespace bbque {

typedef std::list<ResourcePathPtr_t> ResourcePathList_t;
typedef std::shared_ptr<ResourcePathList_t> ResourcePathListPtr_t;
typedef std::vector<uint8_t> VectorUInt8_t;
typedef std::shared_ptr<VectorUInt8_t> VectorUInt8Ptr_t;
typedef std::map<ResourceIdentifier::Type_t, VectorUInt8Ptr_t> ResourceTypeIDMap_t;
typedef std::map<ResourceIdentifier::Type_t, ResourcePathListPtr_t> ResourceTypePathMap_t;

class OpenCLProxy: public Worker {

public:

	enum ExitCode_t {
		SUCCESS,
		PLATFORM_ERROR,
		DEVICE_ERROR
	};

	/**
	 * @brief Constructor
	 */
	static OpenCLProxy & GetInstance();

	~OpenCLProxy();

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

	/**
	 * @brief Set of OpenCL device resource path for a given type
	 */
	ResourcePathListPtr_t GetDevicePaths(ResourceIdentifier::Type_t r_type);

private:

	/*** Constructor */
	OpenCLProxy();

	/*** Logger instance */
	LoggerIF * logger;

	/*** Number of platforms */
	cl_uint num_platforms = 0;
	/*** Number of devices   */
	cl_uint num_devices   = 0;

	/*** Platform descriptors */
	cl_platform_id * platforms = nullptr;
	/*** Device descriptors   */
	cl_device_id   * devices   = nullptr;

	/*** Map with all the device IDs for each type available   */
	ResourceTypeIDMap_t   device_ids;
	/*** Map with all the device paths for each type available */
	ResourceTypePathMap_t device_paths;

	/** Retrieve the iterator for the vector of device IDs, given a type */
	ResourceTypeIDMap_t::iterator GetDeviceIterator(
		ResourceIdentifier::Type_t r_type);

#ifdef CONFIG_BBQUE_PIL_GPU_PM
	/*** Resource path prefix for the power manager instance */
	ResourcePathPtr_t gpu_rp;

	/*** Power Manager instance */
	PowerManager & pm;

	/*** Initial setup of hardware parameters */
	void HwSetup();
#endif

	/**
	 * @brief Append device ID per device type
	 *
	 * @param r_type The resource type (usually Resource::CPU or Resource::GPU)
	 * @param dev_id The OpenCL device ID
	 */
	void InsertDeviceID(ResourceIdentifier::Type_t r_type, uint8_t dev_id);

	/**
	 * @brief Append resource path per device type
	 *
	 * @param r_type The resource type (usually Resource::CPU or Resource::GPU)
	 * @param dev_p_str A resource path referencing a device of the type in the key
	 */
	void InsertDevicePath(
		ResourceIdentifier::Type_t r_type, std::string const & dev_rp_str);

	/**
	 * @brief Register device resources
	 */
	ExitCode_t RegisterDevices();

	/**
	 * @brief The OpenCL platform monitoring thread
	 *
	 * Peridic calls to the platform-specific power management support can
	 * be done (if enabled)
	 */
	void Task();

};

} // namespace bbque

#endif
