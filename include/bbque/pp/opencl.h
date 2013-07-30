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

#include <string>

#include <CL/cl.h>

#include "bbque/modules_factory.h"
#include "bbque/app/application.h"

#define BBQUE_OCL_PLATFORM_NAME "Advanced Micro Devices, Inc."


using bbque::app::AppPtr_t;
using bbque::plugins::LoggerIF;


namespace bbque {

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

private:

	LoggerIF * logger;

	/** OpenCL platforms */
	cl_uint num_platforms;
	cl_platform_id * platforms;

	/** OpenCL devices */
	cl_uint num_devices;
	cl_device_id   * devices;

	/**
	 * @brief Register device resources
	 */
	ExitCode_t RegisterDevices();
};

} // namespace bbque

#endif
