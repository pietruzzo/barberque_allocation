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

#include <cstdint>
#include <cstdlib>

#include "bbque/rtlib/bbque_ocl.h"
#include "bbque/utils/utility.h"

#undef  BBQUE_LOG_MODULE
#define BBQUE_LOG_MODULE "rtl.ocl"

extern RTLIB_OpenCL_t rtlib_ocl;

/* Platform API */
extern CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(
		cl_uint num_entries,
		cl_platform_id *platforms,
		cl_uint *num_platforms)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetPlatformIDs()...\n"));
	return rtlib_ocl.getPlatformIDs(num_entries, platforms, num_platforms);
}
