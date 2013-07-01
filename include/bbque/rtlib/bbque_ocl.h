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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RTLIB_OpenCL RTLIB_OpenCL_t;

struct RTLIB_OpenCL {
	cl_int (*getPlatformIDs)(cl_uint, cl_platform_id *, cl_uint *);
	cl_int (*getPlatformInfo)(cl_platform_id, cl_platform_info, size_t, void *, size_t *);
	cl_int (*getDeviceIDs)(cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *);
	cl_int (*getDeviceInfo)(cl_device_id, cl_device_info, size_t, void *, size_t *);
	cl_int (*createSubDevices)(cl_device_id, const cl_device_partition_property *,
		cl_uint, cl_device_id *, cl_uint *);
	cl_int (*retainDevice)(cl_device_id);
	cl_int (*releaseDevice)(cl_device_id);
	cl_context (*createContext)(const cl_context_properties *, cl_uint, const cl_device_id *,
		void (CL_CALLBACK *)(const char *, const void *, size_t, void *),
		void *, cl_int *);
	cl_context (*createContextFromType)(const cl_context_properties *, cl_device_type,
		void (CL_CALLBACK *)(const char *, const void *, size_t, void *),
		void *, cl_int *);
	cl_int (*retainContext)(cl_context);
	cl_int (*releaseContext)(cl_context);
	cl_int (*getContextInfo)(cl_context, cl_context_info, size_t, void *, size_t *);
	cl_command_queue (*createCommandQueue)(cl_context, cl_device_id,
		cl_command_queue_properties, cl_int *);
	cl_int (*retainCommandQueue)(cl_command_queue);
	cl_int (*releaseCommandQueue)(cl_command_queue);
	cl_int (*getCommandQueueInfo)(cl_command_queue, cl_command_queue_info,
		size_t, void *, size_t *);
	cl_mem (*createBuffer)(cl_context, cl_mem_flags, size_t, void *, cl_int *);
	cl_mem (*createSubBuffer)(cl_mem, cl_mem_flags, cl_buffer_create_type, const void *, cl_int *);
	cl_mem (*createImage)(cl_context, cl_mem_flags, const cl_image_format *, const cl_image_desc *, void *, cl_int *);
	cl_int (*retainMemObject)(cl_mem);
	cl_int (*releaseMemObject)(cl_mem);
	cl_int (*getSupportedImageFormats)(cl_context, cl_mem_flags, cl_mem_object_type, cl_uint, cl_image_format *, cl_uint *);
	cl_int (*getMemObjectInfo)(cl_mem, cl_mem_info, size_t, void *, size_t *);
	cl_int (*getImageInfo)(cl_mem, cl_image_info, size_t, void *, size_t *);
	cl_int (*setMemObjectDestructorCallback)(cl_mem, void (CL_CALLBACK *)(cl_mem, void*), void *);
	cl_sampler (*createSampler)(cl_context, cl_bool, cl_addressing_mode, cl_filter_mode, cl_int *);
	cl_int (*retainSampler)(cl_sampler);
	cl_int (*releaseSampler)(cl_sampler);
	cl_int (*getSamplerInfo)(cl_sampler, cl_sampler_info, size_t,void *, size_t *);
	cl_program (*createProgramWithSource)(cl_context, cl_uint, const char **, const size_t *, cl_int *);
	cl_program (*createProgramWithBinary)(cl_context, cl_uint, const cl_device_id *, const size_t *, const unsigned char **, cl_int *, cl_int *);
	cl_program (*createProgramWithBuiltInKernels)(cl_context, cl_uint, const cl_device_id *, const char *, cl_int *);
	cl_int (*retainProgram)(cl_program);
	cl_int (*releaseProgram)(cl_program);
	cl_int (*buildProgram)(cl_program, cl_uint, const cl_device_id *, const char *, void (CL_CALLBACK *)(cl_program, void *), void *);
	cl_int (*compileProgram)(cl_program, cl_uint, const cl_device_id *, const char *, cl_uint, const cl_program *, const char **, void (CL_CALLBACK *)(cl_program, void *), void *);
	cl_program (*linkProgram)(cl_context, cl_uint, const cl_device_id *, const char *, cl_uint, const cl_program *, void (CL_CALLBACK *)(cl_program, void *), void *, cl_int *);
	cl_int (*unloadPlatformCompiler)(cl_platform_id);
	cl_int (*getProgramInfo)(cl_program, cl_program_info, size_t, void *, size_t *);
	cl_int (*getProgramBuildInfo)(cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *);
	cl_kernel (*createKernel)(cl_program, const char *, cl_int *);
	cl_int (*createKernelsInProgram)(cl_program, cl_uint, cl_kernel *, cl_uint *);
	cl_int (*retainKernel)(cl_kernel);
	cl_int (*releaseKernel)(cl_kernel);
	cl_int (*setKernelArg)(cl_kernel, cl_uint, size_t, const void *);
	cl_int (*getKernelInfo)(cl_kernel, cl_kernel_info, size_t, void *, size_t *);
	cl_int (*getKernelArgInfo)(cl_kernel, cl_uint, cl_kernel_arg_info, size_t, void *, size_t *);
	cl_int (*getKernelWorkGroupInfo)(cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void *, size_t *);
	cl_int (*waitForEvents)(cl_uint, const cl_event *);
	cl_int (*getEventInfo)(cl_event, cl_event_info, size_t, void *, size_t *);
	cl_event (*createUserEvent)(cl_context, cl_int *);
	cl_int (*retainEvent)(cl_event);
	cl_int (*releaseEvent)(cl_event);
	cl_int (*setUserEventStatus)(cl_event, cl_int);
	cl_int (*setEventCallback)(cl_event, cl_int, void (CL_CALLBACK *)(cl_event, cl_int, void *),
		void *);
	cl_int (*getEventProfilingInfo)(cl_event, cl_profiling_info, size_t, void *, size_t *);
	cl_int (*flush)(cl_command_queue);
	cl_int (*finish)(cl_command_queue);
};

/* Platform API */
extern CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(
		cl_uint,
		cl_platform_id *,
		cl_uint *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformInfo(
		cl_platform_id,
		cl_platform_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

/* Device APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDs(
		cl_platform_id,
		cl_device_type,
		cl_uint,
		cl_device_id *,
		cl_uint *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceInfo(
		cl_device_id,
		cl_device_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clCreateSubDevices(
		cl_device_id,
		const cl_device_partition_property *,
		cl_uint,
		cl_device_id *,
		cl_uint *)
		CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainDevice(cl_device_id) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseDevice(cl_device_id) CL_API_SUFFIX__VERSION_1_2;

/* Context APIs  */
extern CL_API_ENTRY cl_context CL_API_CALL
clCreateContext(
		const cl_context_properties *,
		cl_uint,
		const cl_device_id *,
		void (CL_CALLBACK *)(const char *, const void *, size_t, void *),
		void *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromType(
		const cl_context_properties *,
		cl_device_type,
		void (CL_CALLBACK *)(const char *, const void *, size_t, void *),
		void *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainContext(cl_context) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseContext(cl_context) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo(
		cl_context,
        cl_context_info,
        size_t,
        void *,
        size_t *)
        CL_API_SUFFIX__VERSION_1_0;

/* Command Queue APIs */
extern CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue(
		cl_context,
		cl_device_id,
		cl_command_queue_properties,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue(cl_command_queue) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo(
		cl_command_queue,
		cl_command_queue_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

/* Memory Object APIs */
extern CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer(
		cl_context,
		cl_mem_flags,
		size_t,
		void *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_mem CL_API_CALL
clCreateSubBuffer(
		cl_mem,
		cl_mem_flags,
		cl_buffer_create_type,
		const void *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage(
		cl_context,
		cl_mem_flags,
		const cl_image_format *,
		const cl_image_desc *,
		void *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject(cl_mem) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject(cl_mem) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetSupportedImageFormats(
		cl_context,
		cl_mem_flags,
		cl_mem_object_type,
		cl_uint,
		cl_image_format *,
		cl_uint *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(
		cl_mem,
		cl_mem_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetImageInfo(
		cl_mem,
		cl_image_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clSetMemObjectDestructorCallback(
		cl_mem,
		void (CL_CALLBACK *)(cl_mem, void*), void *)
		CL_API_SUFFIX__VERSION_1_1;

/* Sampler APIs */
extern CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSampler(
		cl_context,
		cl_bool,
		cl_addressing_mode,
		cl_filter_mode,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainSampler(cl_sampler) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseSampler(cl_sampler) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetSamplerInfo(
		cl_sampler,
		cl_sampler_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

/* Program Object APIs  */
extern CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithSource(
		cl_context,
		cl_uint,
		const char **,
		const size_t *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBinary(
		cl_context,
		cl_uint,
		const cl_device_id *,
		const size_t *,
		const unsigned char **,
		cl_int *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBuiltInKernels(
		cl_context,
		cl_uint,
		const cl_device_id *,
		const char *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainProgram(cl_program) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseProgram(cl_program) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clBuildProgram(
		cl_program,
		cl_uint,
		const cl_device_id *,
		const char *,
		void (CL_CALLBACK *)(cl_program, void *),
		void *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clCompileProgram(
		cl_program,
		cl_uint,
		const cl_device_id *,
		const char *,
		cl_uint,
		const cl_program *,
		const char **,
		void (CL_CALLBACK *)(cl_program, void *),
		void *)
		CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_program CL_API_CALL
clLinkProgram(
		cl_context,
		cl_uint,
		const cl_device_id *,
		const char *,
		cl_uint,
		const cl_program *,
		void (CL_CALLBACK *)(cl_program, void *),
		void *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
clUnloadPlatformCompiler(cl_platform_id) CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetProgramInfo(
		cl_program,
		cl_program_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetProgramBuildInfo(
		cl_program,
		cl_device_id,
		cl_program_build_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

/* Kernel Object APIs */
extern CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel(
		cl_program,
		const char *,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(
		cl_program,
		cl_uint,
		cl_kernel *,
		cl_uint *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel(cl_kernel) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel(cl_kernel) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg(
		cl_kernel,
		cl_uint,
		size_t,
		const void *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo(
		cl_kernel,
		cl_kernel_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetKernelArgInfo(
		cl_kernel,
		cl_uint,
		cl_kernel_arg_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_2;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetKernelWorkGroupInfo(
		cl_kernel,
		cl_device_id,
		cl_kernel_work_group_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

/* Event Object APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(
		cl_uint,
		const cl_event *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(
		cl_event,
		cl_event_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_event CL_API_CALL
clCreateUserEvent(
		cl_context,
		cl_int *)
		CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clSetUserEventStatus(
		cl_event,
		cl_int)
		CL_API_SUFFIX__VERSION_1_1;

extern CL_API_ENTRY cl_int CL_API_CALL
clSetEventCallback(
		cl_event,
		cl_int,
		void (CL_CALLBACK *)(cl_event, cl_int, void *),
		void *)
		CL_API_SUFFIX__VERSION_1_1;

/* Profiling APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo(
		cl_event,
		cl_profiling_info,
		size_t,
		void *,
		size_t *)
		CL_API_SUFFIX__VERSION_1_0;

/* Flush and Finish APIs */
extern CL_API_ENTRY cl_int CL_API_CALL
clFlush(cl_command_queue) CL_API_SUFFIX__VERSION_1_0;

extern CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue) CL_API_SUFFIX__VERSION_1_0;


#ifdef  __cplusplus
}
#endif

#endif // BBQUE_OCL_H_

