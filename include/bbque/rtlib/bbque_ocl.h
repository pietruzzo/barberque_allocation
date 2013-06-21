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

#ifndef BBQUE_OCL_H_
#define BBQUE_OCL_H_

#include <CL/cl.h>

typedef struct RTLIB_OpenCL RTLIB_OpenCL_t;

struct RTLIB_OpenCL {
	cl_int (*getPlatformIDs) (cl_uint, cl_platform_id *, cl_uint *);
};

/* Platform API */
extern CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(cl_uint, cl_platform_id *, cl_uint *) CL_API_SUFFIX__VERSION_1_0;

#endif // BBQUE_OCL_H_

