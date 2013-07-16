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
#include <dlfcn.h>

#include <cstdint>
#include <cstdlib>

#include "bbque/rtlib/bbque_ocl.h"
#include "bbque/utils/utility.h"

#undef  BBQUE_LOG_MODULE
#define BBQUE_LOG_MODULE "rtl.ocl"

#ifdef __cplusplus
extern "C" {
#endif

extern RTLIB_OpenCL_t rtlib_ocl;
extern std::map<cl_command_queue, QueueProfPtr_t> ocl_queues_prof;
extern std::map<cl_command_type, std::string> ocl_cmd_str;

/* Platform API */
CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformIDs(
		cl_uint num_entries,
		cl_platform_id *platforms,
		cl_uint *num_platforms)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetPlatformIDs()...\n"));
	return rtlib_ocl.getPlatformIDs(num_entries, platforms, num_platforms);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetPlatformInfo(
		cl_platform_id platform,
		cl_platform_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetPlatformInfo()...\n"));
	return rtlib_ocl.getPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret);
}

/* Device APIs */
CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceIDs(
		cl_platform_id platform,
		cl_device_type device_type,
		cl_uint num_entries,
		cl_device_id *devices,
		cl_uint *num_devices)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetDeviceIDs()...\n"));
	return rtlib_ocl.getDeviceIDs(platform, device_type, num_entries, devices, num_devices);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceInfo(
		cl_device_id device,
		cl_device_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetDeviceInfo()...\n"));
	return rtlib_ocl.getDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateSubDevices(
		cl_device_id in_device,
		const cl_device_partition_property *properties,
		cl_uint num_devices,
		cl_device_id *out_devices,
		cl_uint *num_devices_ret)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clCreateSubDevices()...\n"));
	return rtlib_ocl.createSubDevices(in_device, properties, num_devices, out_devices, num_devices_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainDevice(cl_device_id device) CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clRetainDevice()...\n"));
	return rtlib_ocl.retainDevice(device);
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseDevice(cl_device_id device) CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clReleaseDevice()...\n"));
	return rtlib_ocl.releaseDevice(device);
}

/* Context APIs  */
CL_API_ENTRY cl_context CL_API_CALL
clCreateContext(
		const cl_context_properties *properties,
		cl_uint num_devices,
		const cl_device_id *devices,
		void (CL_CALLBACK *pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
		void *user_data,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clCreateContext()...\n"));
	return rtlib_ocl.createContext(properties, num_devices, devices, (*pfn_notify),
		user_data, errcode_ret);
}

CL_API_ENTRY cl_context CL_API_CALL
clCreateContextFromType(
		const cl_context_properties *properties,
		cl_device_type device_type,
		void (CL_CALLBACK *pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
		void *user_data,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clCreateContextFromType()...\n"));
	return rtlib_ocl.createContextFromType(properties, device_type, (*pfn_notify),
		user_data, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainContext(cl_context context) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clRetainContext()...\n"));
	return rtlib_ocl.retainContext(context);
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseContext(cl_context context) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clReleaseContext()...\n"));
	return rtlib_ocl.releaseContext(context);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetContextInfo(
		cl_context context,
        cl_context_info param_name,
        size_t param_value_size,
        void *param_value,
        size_t *param_value_size_ret)
        CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetContextInfo()...\n"));
	return rtlib_ocl.getContextInfo(context, param_name, param_value_size,
		param_value, param_value_size_ret);
}

/* Command Queue APIs */
CL_API_ENTRY cl_command_queue CL_API_CALL
clCreateCommandQueue(
		cl_context context,
		cl_device_id device,
		cl_command_queue_properties properties,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clCreateCommandQueue()...\n"));
	return rtlib_ocl.createCommandQueue(context, device, properties, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clRetainCommandQueue()...\n"));
	return rtlib_ocl.retainCommandQueue(command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandQueue(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clReleaseCommandQueue()...\n"));
	return rtlib_ocl.releaseCommandQueue(command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetCommandQueueInfo(
		cl_command_queue command_queue,
		cl_command_queue_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetCommandQueueInfo()...\n"));
	return rtlib_ocl.getCommandQueueInfo(command_queue, param_name,
		param_value_size, param_value, param_value_size_ret);
}

/* Memory Object APIs */
CL_API_ENTRY cl_mem CL_API_CALL
clCreateBuffer(
		cl_context context,
		cl_mem_flags flags,
		size_t size,
		void *host_ptr,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clCreateBuffer()...\n"));
	return rtlib_ocl.createBuffer(context, flags, size, host_ptr, errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateSubBuffer(
		cl_mem buffer,
		cl_mem_flags flags,
		cl_buffer_create_type buffer_create_type,
		const void *buffer_create_info,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_1 {
	fprintf(stderr, FD("Calling clCreateSubBuffer()...\n"));
	return rtlib_ocl.createSubBuffer(buffer, flags, buffer_create_type,
		buffer_create_info, errcode_ret);
}

CL_API_ENTRY cl_mem CL_API_CALL
clCreateImage(
		cl_context context,
		cl_mem_flags flags,
		const cl_image_format *image_format,
		const cl_image_desc *image_desc,
		void *host_ptr,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clCreateImage()...\n"));
	return rtlib_ocl.createImage(context, flags, image_format, image_desc,
		host_ptr, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clRetainMemObject()...\n"));
	return rtlib_ocl.retainMemObject(memobj);
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseMemObject(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clReleaseMemObject()...\n"));
	return rtlib_ocl.releaseMemObject(memobj);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSupportedImageFormats(
		cl_context context,
		cl_mem_flags flags,
		cl_mem_object_type image_type,
		cl_uint num_entries,
		cl_image_format *image_formats,
		cl_uint *num_image_formats)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetSupportedImageFormats()...\n"));
	return rtlib_ocl.getSupportedImageFormats(context, flags, image_type,
		num_entries, image_formats, num_image_formats);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetMemObjectInfo(
		cl_mem memobj,
		cl_mem_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetMemObjectInfo()...\n"));
	return rtlib_ocl.getMemObjectInfo(memobj, param_name, param_value_size,
		param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetImageInfo(
		cl_mem  image,
		cl_image_info param_name,
		size_t param_value_size,
		void * param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetImageInfo()...\n"));
	return rtlib_ocl.getImageInfo(image, param_name, param_value_size,
		param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clSetMemObjectDestructorCallback(
		cl_mem memobj,
		void (CL_CALLBACK *pfn_notify)(cl_mem memobj, void *user_data),
		void *user_data)
		CL_API_SUFFIX__VERSION_1_1 {
	fprintf(stderr, FD("Calling clSetMemObjectDestructorCallback()...\n"));
	return rtlib_ocl.setMemObjectDestructorCallback(memobj,(*pfn_notify), user_data);
}

/* Sampler APIs */
CL_API_ENTRY cl_sampler CL_API_CALL
clCreateSampler(
		cl_context context,
		cl_bool normalized_coords,
		cl_addressing_mode addressing_mode,
		cl_filter_mode filter_mode,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clCreateSampler()...\n"));
	return rtlib_ocl.createSampler(context, normalized_coords, addressing_mode,
		filter_mode, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clRetainSampler()...\n"));
	return rtlib_ocl.retainSampler(sampler);
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseSampler(cl_sampler sampler) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clReleaseSampler()...\n"));
	return rtlib_ocl.releaseSampler(sampler);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetSamplerInfo(
		cl_sampler sampler,
		cl_sampler_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetSamplerInfo()...\n"));
	return rtlib_ocl.getSamplerInfo(sampler, param_name, param_value_size,
		param_value, param_value_size_ret);
}

/* Program Object APIs  */
CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithSource(
		cl_context context,
		cl_uint count,
		const char **strings,
		const size_t *lengths,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clCreateProgramWithSource()...\n"));
	return rtlib_ocl.createProgramWithSource(context, count, strings, lengths, errcode_ret);
}

CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBinary(
		cl_context context,
		cl_uint num_devices,
		const cl_device_id *device_list,
		const size_t *lengths,
		const unsigned char **binaries,
		cl_int *binary_status,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clCreateProgramWithBinary()...\n"));
	return rtlib_ocl.createProgramWithBinary(context, num_devices, device_list,
		lengths, binaries, binary_status, errcode_ret);
}

CL_API_ENTRY cl_program CL_API_CALL
clCreateProgramWithBuiltInKernels(
		cl_context context,
		cl_uint num_devices,
		const cl_device_id *device_list,
		const char *kernel_names,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clCreateProgramWithBuiltInKernels()...\n"));
	return rtlib_ocl.createProgramWithBuiltInKernels(context, num_devices,
		device_list, kernel_names, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clRetainProgram()...\n"));
	return rtlib_ocl.retainProgram(program);
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseProgram(cl_program program) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clReleaseProgram()...\n"));
	return rtlib_ocl.releaseProgram(program);
}

CL_API_ENTRY cl_int CL_API_CALL
clBuildProgram(
		cl_program program,
		cl_uint num_devices,
		const cl_device_id *device_list,
		const char *options,
		void (CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
		void *user_data)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clBuildProgram()...\n"));
	return rtlib_ocl.buildProgram(program, num_devices, device_list, options,
		(*pfn_notify), user_data);
}

CL_API_ENTRY cl_int CL_API_CALL
clCompileProgram(
		cl_program program,
		cl_uint num_devices,
		const cl_device_id *device_list,
		const char *options,
		cl_uint num_input_headers,
		const cl_program *input_headers,
		const char **header_include_names,
		void (CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
		void *user_data)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clCompileProgram()...\n"));
	return rtlib_ocl.compileProgram(program, num_devices, device_list, options,
		num_input_headers, input_headers, header_include_names, (*pfn_notify), user_data);
}

CL_API_ENTRY cl_program CL_API_CALL
clLinkProgram(
		cl_context context,
		cl_uint num_devices,
		const cl_device_id *device_list,
		const char *options,
		cl_uint num_input_programs,
		const cl_program *input_programs,
		void (CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
		void *user_data,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clLinkProgram()...\n"));
	return rtlib_ocl.linkProgram(context, num_devices, device_list, options,
		num_input_programs, input_programs, (*pfn_notify), user_data, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clUnloadPlatformCompiler(cl_platform_id platform) CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clUnloadPlatformCompiler()...\n"));
	return rtlib_ocl.unloadPlatformCompiler(platform);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramInfo(
		cl_program program,
		cl_program_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetProgramInfo()...\n"));
	return rtlib_ocl.getProgramInfo(program, param_name, param_value_size,
		param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetProgramBuildInfo(
		cl_program program,
		cl_device_id device,
		cl_program_build_info param_name,
		size_t  param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetProgramBuildInfo()...\n"));
	return rtlib_ocl.getProgramBuildInfo(program, device, param_name,
		param_value_size, param_value, param_value_size_ret);
}

/* Kernel Object APIs */
CL_API_ENTRY cl_kernel CL_API_CALL
clCreateKernel(
		cl_program program,
		const char *kernel_name,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clCreateKernel()...\n"));
	return rtlib_ocl.createKernel(program, kernel_name, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clCreateKernelsInProgram(
		cl_program program,
		cl_uint num_kernels,
		cl_kernel *kernels,
		cl_uint *num_kernels_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clCreateKernelsInProgram()...\n"));
	return rtlib_ocl.createKernelsInProgram(program, num_kernels, kernels, num_kernels_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainKernel(cl_kernel kernel) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clRetainKernel()...\n"));
	return rtlib_ocl.retainKernel(kernel);
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseKernel(cl_kernel kernel) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clReleaseKernel()...\n"));
	return rtlib_ocl.releaseKernel(kernel);
}

CL_API_ENTRY cl_int CL_API_CALL
clSetKernelArg(
		cl_kernel kernel,
		cl_uint arg_index,
		size_t arg_size,
		const void *arg_value)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clSetKernelArg()...\n"));
	return rtlib_ocl.setKernelArg(kernel, arg_index, arg_size, arg_value);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelInfo(
		cl_kernel kernel,
		cl_kernel_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetKernelInfo()...\n"));
	return rtlib_ocl.getKernelInfo(kernel, param_name, param_value_size,
		param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelArgInfo(
		cl_kernel kernel,
		cl_uint arg_indx,
		cl_kernel_arg_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clGetKernelArgInfo()...\n"));
	return rtlib_ocl.getKernelArgInfo(kernel, arg_indx, param_name,
		param_value_size, param_value, param_value_size_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetKernelWorkGroupInfo(
		cl_kernel kernel,
		cl_device_id device,
		cl_kernel_work_group_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetKernelWorkGroupInfo()...\n"));
	return rtlib_ocl.getKernelWorkGroupInfo(kernel, device, param_name,
		param_value_size, param_value, param_value_size_ret);
}

/* Event Object APIs */
CL_API_ENTRY cl_int CL_API_CALL
clWaitForEvents(
		cl_uint num_events,
		const cl_event *event_list)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clWaitForEvents()...\n"));
	return rtlib_ocl.waitForEvents(num_events, event_list);
}

CL_API_ENTRY cl_int CL_API_CALL
clGetEventInfo(
		cl_event event,
		cl_event_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetEventInfo()...\n"));
	return rtlib_ocl.getEventInfo(event, param_name, param_value_size,
		param_value, param_value_size_ret);
}

CL_API_ENTRY cl_event CL_API_CALL
clCreateUserEvent(
		cl_context context,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_1 {
	fprintf(stderr, FD("Calling clCreateUserEvent()...\n"));
	return rtlib_ocl.createUserEvent(context, errcode_ret);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clRetainEvent()...\n"));
	return rtlib_ocl.retainEvent(event);
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseEvent(cl_event event) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clReleaseEvent()...\n"));
	return rtlib_ocl.releaseEvent(event);
}

CL_API_ENTRY cl_int CL_API_CALL
clSetUserEventStatus(
		cl_event event,
		cl_int execution_status)
		CL_API_SUFFIX__VERSION_1_1 {
	fprintf(stderr, FD("Calling clSetUserEventStatus()...\n"));
	return rtlib_ocl.setUserEventStatus(event, execution_status);
}

CL_API_ENTRY cl_int CL_API_CALL
clSetEventCallback(
		cl_event event,
		cl_int command_exec_callback_type,
		void (CL_CALLBACK *pfn_event_notify)(cl_event event, cl_int event_command_exec_status, void *user_data),
		void *user_data)
		CL_API_SUFFIX__VERSION_1_1 {
	fprintf(stderr, FD("Calling clSetEventCallback()...\n"));
	return rtlib_ocl.setEventCallback(event, command_exec_callback_type,
		(*pfn_event_notify), user_data);
}

/* Profiling APIs */
CL_API_ENTRY cl_int CL_API_CALL
clGetEventProfilingInfo(
		cl_event event,
		cl_profiling_info param_name,
		size_t param_value_size,
		void *param_value,
		size_t *param_value_size_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clGetEventProfilingInfo()...\n"));
	return rtlib_ocl.getEventProfilingInfo(event, param_name, param_value_size,
		param_value, param_value_size_ret);
}

/* Flush and Finish APIs */
CL_API_ENTRY cl_int CL_API_CALL
clFlush(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clFlush()...\n"));
	return rtlib_ocl.flush(command_queue);
}

CL_API_ENTRY cl_int CL_API_CALL
clFinish(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clFinish()...\n"));
	return rtlib_ocl.finish(command_queue);
}

/* Enqueued Commands APIs */
CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBuffer(
		cl_command_queue command_queue,
		cl_mem buffer,
		cl_bool blocking_read,
		size_t offset,
		size_t size,
		void *ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueReadBuffer()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueReadBuffer(command_queue, buffer, blocking_read,
		offset, size, ptr, num_events_in_wait_list, event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueReadBuffer\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadBufferRect(
		cl_command_queue command_queue,
		cl_mem buffer,
		cl_bool blocking_read,
		const size_t *buffer_origin,
		const size_t *host_origin,
		const size_t *region,
		size_t buffer_row_pitch,
		size_t buffer_slice_pitch,
		size_t host_row_pitch,
		size_t host_slice_pitch,
		void *ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_1 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueReadBufferRect()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueReadBufferRect(command_queue, buffer,
		blocking_read, buffer_origin, host_origin, region, buffer_row_pitch,
		buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr,
		num_events_in_wait_list, event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueReadBufferRect\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBuffer(
		cl_command_queue command_queue,
		cl_mem buffer,
		cl_bool blocking_write,
		size_t offset,
		size_t size,
		const void *ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueWriteBuffer()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueWriteBuffer(command_queue, buffer, blocking_write,
		offset, size, ptr, num_events_in_wait_list, event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueWriteBuffer\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteBufferRect(
		cl_command_queue command_queue,
		cl_mem buffer,
		cl_bool blocking_write,
		const size_t *buffer_origin,
		const size_t *host_origin,
		const size_t *region,
		size_t buffer_row_pitch,
		size_t buffer_slice_pitch,
		size_t host_row_pitch,
		size_t host_slice_pitch,
		const void *ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_1 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueWriteBufferRect()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueWriteBufferRect(command_queue, buffer, blocking_write,
		buffer_origin, host_origin, region,	buffer_row_pitch, buffer_slice_pitch,
		host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list,
		event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueWriteBufferRect\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueFillBuffer(
		cl_command_queue command_queue,
		cl_mem buffer,
		const void *pattern,
		size_t pattern_size,
		size_t offset,
		size_t size,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clEnqueueFillBuffer()...\n"));
	return rtlib_ocl.enqueueFillBuffer(command_queue, buffer, pattern, pattern_size,
		offset, size, num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBuffer(
		cl_command_queue command_queue,
		cl_mem src_buffer,
		cl_mem dst_buffer,
		size_t src_offset,
		size_t dst_offset,
		size_t size,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueCopyBuffer()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueCopyBuffer(command_queue, src_buffer, dst_buffer,
		src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueCopyBuffer\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferRect(
		cl_command_queue command_queue,
		cl_mem src_buffer,
		cl_mem dst_buffer,
		const size_t *src_origin,
		const size_t *dst_origin,
		const size_t *region,
		size_t src_row_pitch,
		size_t src_slice_pitch,
		size_t dst_row_pitch,
		size_t dst_slice_pitch,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_1 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueCopyBufferRect()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueCopyBufferRect(command_queue, src_buffer, dst_buffer,
		src_origin, dst_origin, region, src_row_pitch, src_slice_pitch,
		dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueCopyBufferRect\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueReadImage(
		cl_command_queue command_queue,
		cl_mem image,
		cl_bool blocking_read,
		const size_t *origin,
		const size_t *region,
		size_t row_pitch,
		size_t slice_pitch,
		void *ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueReadImage()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueReadImage(command_queue, image, blocking_read,
		origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list,
		event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueReadImage\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueWriteImage(
		cl_command_queue command_queue,
		cl_mem image,
		cl_bool blocking_write,
		const size_t *origin,
		const size_t *region,
		size_t input_row_pitch,
		size_t input_slice_pitch,
		const void *ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueWriteImage()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueWriteImage(command_queue, image, blocking_write,
		origin, region, input_row_pitch, input_slice_pitch, ptr,
		num_events_in_wait_list, event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueWriteImage\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueFillImage(
		cl_command_queue command_queue,
		cl_mem image,
		const void *fill_color,
		const size_t *origin,
		const size_t *region,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clEnqueueFillImage()...\n"));
	return rtlib_ocl.enqueueFillImage(command_queue, image, fill_color, origin,
		region, num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImage(
		cl_command_queue command_queue,
		cl_mem src_image,
		cl_mem dst_image,
		const size_t *src_origin,
		const size_t *dst_origin,
		const size_t *region,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueCopyImage()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueCopyImage(command_queue, src_image, dst_image,
		src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueCopyImage\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyImageToBuffer(
		cl_command_queue command_queue,
		cl_mem src_image,
		cl_mem dst_buffer,
		const size_t *src_origin,
		const size_t *region,
		size_t dst_offset,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueCopyImageToBuffer()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueCopyImageToBuffer(command_queue, src_image,
		dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list,
		event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueCopyImageToBuffer\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCopyBufferToImage(
		cl_command_queue command_queue,
		cl_mem src_buffer,
		cl_mem dst_image,
		size_t src_offset,
		const size_t *dst_origin,
		const size_t *region,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueCopyBufferToImage()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueCopyBufferToImage(command_queue, src_buffer,
		dst_image, src_offset, dst_origin, region, num_events_in_wait_list,
		event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueCopyBufferToImage\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapBuffer(
		cl_command_queue command_queue,
		cl_mem buffer,
		cl_bool blocking_map,
		cl_map_flags map_flags,
		size_t offset,
		size_t size,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	void * buff_ptr;
	fprintf(stderr, FD("Calling clEnqueueMapBuffer()...\n"));
	EVENT_RC_CONTROL(event);
	buff_ptr = rtlib_ocl.enqueueMapBuffer(command_queue, buffer, blocking_map,
		map_flags, offset, size, num_events_in_wait_list, event_wait_list,
		event, errcode_ret);
	if (errcode_ret != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueMapBuffer\n"), errcode_ret);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return buff_ptr;
}

CL_API_ENTRY void * CL_API_CALL
clEnqueueMapImage(
		cl_command_queue command_queue,
		cl_mem image,
		cl_bool blocking_map,
		cl_map_flags map_flags,
		const size_t *origin,
		const size_t *region,
		size_t *image_row_pitch,
		size_t *image_slice_pitch,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event,
		cl_int *errcode_ret)
		CL_API_SUFFIX__VERSION_1_0 {
	void * buff_ptr;
	fprintf(stderr, FD("Calling clEnqueueMapImage()...\n"));
	EVENT_RC_CONTROL(event);
	buff_ptr = rtlib_ocl.enqueueMapImage(command_queue, image, blocking_map,
		map_flags, origin, region, image_row_pitch, image_slice_pitch,
		num_events_in_wait_list, event_wait_list, event, errcode_ret);
	if (errcode_ret != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueMapImage\n"), errcode_ret);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return buff_ptr;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueUnmapMemObject(
		cl_command_queue command_queue,
		cl_mem memobj,
		void *mapped_ptr,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	fprintf(stderr, FD("Calling clEnqueueUnmapMemObject()...\n"));
	return rtlib_ocl.enqueueUnmapMemObject(command_queue, memobj, mapped_ptr,
		num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMigrateMemObjects(
		cl_command_queue command_queue,
		cl_uint num_mem_objects,
		const cl_mem *mem_objects,
		cl_mem_migration_flags flags,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clEnqueueMigrateMemObjects()...\n"));
	return rtlib_ocl.enqueueMigrateMemObjects(command_queue, num_mem_objects,
		mem_objects, flags, num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNDRangeKernel(
		cl_command_queue command_queue,
		cl_kernel kernel,
		cl_uint work_dim,
		const size_t *global_work_offset,
		const size_t *global_work_size,
		const size_t *local_work_size,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueNDRangeKernel()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueNDRangeKernel(command_queue, kernel, work_dim,
		global_work_offset, global_work_size, local_work_size,
		num_events_in_wait_list, event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueNDRangeKernel\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueTask(
		cl_command_queue command_queue,
		cl_kernel kernel,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueTask()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueTask(command_queue, kernel, num_events_in_wait_list,
		event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueTask\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueNativeKernel(
		cl_command_queue command_queue,
		void (CL_CALLBACK *user_func)(void *),
		void *args,
		size_t cb_args,
		cl_uint num_mem_objects,
		const cl_mem *mem_list,
		const void **args_mem_loc,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_0 {
	cl_int status;
	fprintf(stderr, FD("Calling clEnqueueNativeKernel()...\n"));
	EVENT_RC_CONTROL(event);
	status = rtlib_ocl.enqueueNativeKernel(command_queue, (*user_func), args,
		cb_args, num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list,
		event_wait_list, event);
	if (status != CL_SUCCESS) {
		fprintf(stderr, FE("OCL: Error [%d] in clEnqueueNativeKernel\n"), status);
	}
	rtlib_ocl_coll_event(command_queue, event);
	return status;
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueMarkerWithWaitList(
		cl_command_queue command_queue,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clEnqueueMarkerWithWaitList()...\n"));
	return rtlib_ocl.enqueueMarkerWithWaitList(command_queue, num_events_in_wait_list,
		event_wait_list, event);
}

CL_API_ENTRY cl_int CL_API_CALL
clEnqueueBarrierWithWaitList(
		cl_command_queue command_queue,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)
		CL_API_SUFFIX__VERSION_1_2 {
	fprintf(stderr, FD("Calling clEnqueueBarrierWithWaitList()...\n"));
	return rtlib_ocl.enqueueBarrierWithWaitList(command_queue,
	num_events_in_wait_list, event_wait_list, event);
}

// Initialize OpenCL wrappers
void rtlib_ocl_init() {
	void *handle = dlopen("/opt/AMDAPP/lib/x86_64/libOpenCL.so", RTLD_LOCAL | RTLD_LAZY);

	rtlib_ocl.getPlatformIDs = (getPlatformIDs_t) dlsym(handle, "clGetPlatformIDs");
	rtlib_ocl.getPlatformInfo = (getPlatformInfo_t) dlsym(handle, "clGetPlatformInfo");
	rtlib_ocl.getDeviceIDs = (getDeviceIDs_t) dlsym(handle, "clGetDeviceIDs");
	rtlib_ocl.getDeviceInfo = (getDeviceInfo_t) dlsym(handle, "clGetDeviceInfo");
	rtlib_ocl.createSubDevices = (createSubDevices_t) dlsym(handle, "clCreateSubDevices");
	rtlib_ocl.retainDevice = (retainDevice_t) dlsym(handle, "clRetainDevice");
	rtlib_ocl.releaseDevice = (releaseDevice_t) dlsym(handle, "clReleaseDevice");
	rtlib_ocl.createContext = (createContext_t) dlsym(handle, "clCreateContext");
	rtlib_ocl.createContextFromType = (createContextFromType_t) dlsym(handle, "clCreateContextFromType");
	rtlib_ocl.retainContext = (retainContext_t) dlsym(handle, "clRetainContext");
	rtlib_ocl.releaseContext = (releaseContext_t) dlsym(handle, "clReleaseContext");
	rtlib_ocl.getContextInfo = (getContextInfo_t) dlsym(handle, "clGetContextInfo");
	rtlib_ocl.createCommandQueue = (createCommandQueue_t) dlsym(handle, "clCreateCommandQueue");
	rtlib_ocl.retainCommandQueue = (retainCommandQueue_t) dlsym(handle, "clRetainCommandQueue");
	rtlib_ocl.releaseCommandQueue = (releaseCommandQueue_t) dlsym(handle, "clReleaseCommandQueue");
	rtlib_ocl.getCommandQueueInfo = (getCommandQueueInfo_t) dlsym(handle, "clGetCommandQueueInfo");
	rtlib_ocl.createBuffer = (createBuffer_t) dlsym(handle, "clCreateBuffer");
	rtlib_ocl.createSubBuffer = (createSubBuffer_t) dlsym(handle, "clCreateSubBuffer");
	rtlib_ocl.createImage = (createImage_t) dlsym(handle, "clCreateImage");
	rtlib_ocl.retainMemObject = (retainMemObject_t) dlsym(handle, "clRetainMemObject");
	rtlib_ocl.releaseMemObject = (releaseMemObject_t) dlsym(handle, "clReleaseMemObject");
	rtlib_ocl.getSupportedImageFormats = (getSupportedImageFormats_t) dlsym(handle, "clGetSupportedImageFormats");
	rtlib_ocl.getMemObjectInfo = (getMemObjectInfo_t) dlsym(handle, "clGetMemObjectInfo");
	rtlib_ocl.getImageInfo = (getImageInfo_t) dlsym(handle, "clGetImageInfo");
	rtlib_ocl.setMemObjectDestructorCallback = (setMemObjectDestructorCallback_t) dlsym(handle, "clSetMemObjectDestructorCallback");
	rtlib_ocl.createSampler = (createSampler_t) dlsym(handle, "clCreateSampler");
	rtlib_ocl.retainSampler = (retainSampler_t) dlsym(handle, "clRetainSampler");
	rtlib_ocl.releaseSampler = (releaseSampler_t) dlsym(handle, "clReleaseSampler");
	rtlib_ocl.getSamplerInfo = (getSamplerInfo_t) dlsym(handle, "clGetSamplerInfo");
	rtlib_ocl.createProgramWithSource = (createProgramWithSource_t) dlsym(handle, "clCreateProgramWithSource");
	rtlib_ocl.createProgramWithBinary = (createProgramWithBinary_t) dlsym(handle, "clCreateProgramWithBinary");
	rtlib_ocl.createProgramWithBuiltInKernels = (createProgramWithBuiltInKernels_t) dlsym(handle, "clCreateProgramWithBuiltInKernels");
	rtlib_ocl.retainProgram = (retainProgram_t) dlsym(handle, "clRetainProgram");
	rtlib_ocl.releaseProgram = (releaseProgram_t) dlsym(handle, "clReleaseProgram");
	rtlib_ocl.buildProgram = (buildProgram_t) dlsym(handle, "clBuildProgram");
	rtlib_ocl.compileProgram = (compileProgram_t) dlsym(handle, "clCompileProgram");
	rtlib_ocl.linkProgram = (linkProgram_t) dlsym(handle, "clLinkProgram");
	rtlib_ocl.unloadPlatformCompiler = (unloadPlatformCompiler_t) dlsym(handle, "clUnloadPlatformCompiler");
	rtlib_ocl.getProgramInfo = (getProgramInfo_t) dlsym(handle, "clGetProgramInfo");
	rtlib_ocl.getProgramBuildInfo = (getProgramBuildInfo_t) dlsym(handle, "clGetProgramBuildInfo");
	rtlib_ocl.createKernel = (createKernel_t) dlsym(handle, "clCreateKernel");
	rtlib_ocl.createKernelsInProgram = (createKernelsInProgram_t) dlsym(handle, "clCreateKernelsInProgram");
	rtlib_ocl.retainKernel = (retainKernel_t) dlsym(handle, "clRetainKernel");
	rtlib_ocl.releaseKernel = (releaseKernel_t) dlsym(handle, "clReleaseKernel");
	rtlib_ocl.setKernelArg = (setKernelArg_t) dlsym(handle, "clSetKernelArg");
	rtlib_ocl.getKernelInfo = (getKernelInfo_t) dlsym(handle, "clGetKernelInfo");
	rtlib_ocl.getKernelArgInfo = (getKernelArgInfo_t) dlsym(handle, "clGetKernelArgInfo");
	rtlib_ocl.getKernelWorkGroupInfo = (getKernelWorkGroupInfo_t) dlsym(handle, "clGetKernelWorkGroupInfo");
	rtlib_ocl.waitForEvents = (waitForEvents_t) dlsym(handle, "clWaitForEvents");
	rtlib_ocl.getEventInfo = (getEventInfo_t) dlsym(handle, "clGetEventInfo");
	rtlib_ocl.createUserEvent = (createUserEvent_t) dlsym(handle, "clCreateUserEvent");
	rtlib_ocl.retainEvent = (retainEvent_t) dlsym(handle, "clRetainEvent");
	rtlib_ocl.releaseEvent = (releaseEvent_t) dlsym(handle, "clReleaseEvent");
	rtlib_ocl.setUserEventStatus = (setUserEventStatus_t) dlsym(handle, "clSetUserEventStatus");
	rtlib_ocl.setEventCallback = (setEventCallback_t) dlsym(handle, "clSetEventCallback");
	rtlib_ocl.getEventProfilingInfo = (getEventProfilingInfo_t) dlsym(handle, "clGetEventProfilingInfo");
	rtlib_ocl.enqueueReadBuffer = (enqueueReadBuffer_t) dlsym(handle, "clEnqueueReadBuffer");
	rtlib_ocl.enqueueReadBufferRect = (enqueueReadBufferRect_t) dlsym(handle, "clEnqueueReadBufferRect");
	rtlib_ocl.enqueueWriteBuffer = (enqueueWriteBuffer_t) dlsym(handle, "clEnqueueWriteBuffer");
	rtlib_ocl.enqueueWriteBufferRect = (enqueueWriteBufferRect_t) dlsym(handle, "clEnqueueWriteBufferRect");
	rtlib_ocl.enqueueFillBuffer = (enqueueFillBuffer_t) dlsym(handle, "clEnqueueFillBuffer");
	rtlib_ocl.enqueueCopyBuffer = (enqueueCopyBuffer_t) dlsym(handle, "clEnqueueCopyBuffer");
	rtlib_ocl.enqueueCopyBufferRect = (enqueueCopyBufferRect_t) dlsym(handle, "clEnqueueCopyBufferRect");
	rtlib_ocl.enqueueReadImage = (enqueueReadImage_t) dlsym(handle, "clEnqueueReadImage");
	rtlib_ocl.enqueueWriteImage = (enqueueWriteImage_t) dlsym(handle, "clEnqueueWriteImage");
	rtlib_ocl.enqueueFillImage = (enqueueFillImage_t) dlsym(handle, "clEnqueueFillImage");
	rtlib_ocl.enqueueCopyImage = (enqueueCopyImage_t) dlsym(handle, "clEnqueueCopyImage");
	rtlib_ocl.enqueueCopyImageToBuffer = (enqueueCopyImageToBuffer_t) dlsym(handle, "clEnqueueCopyImageToBuffer");
	rtlib_ocl.enqueueCopyBufferToImage = (enqueueCopyBufferToImage_t) dlsym(handle, "clEnqueueCopyBufferToImage");
	rtlib_ocl.enqueueMapBuffer = (enqueueMapBuffer_t) dlsym(handle, "clEnqueueMapBuffer");
	rtlib_ocl.enqueueMapImage = (enqueueMapImage_t) dlsym(handle, "clEnqueueMapImage");
	rtlib_ocl.enqueueUnmapMemObject = (enqueueUnmapMemObject_t) dlsym(handle, "clEnqueueUnmapMemObject");
	rtlib_ocl.enqueueMigrateMemObjects = (enqueueMigrateMemObjects_t) dlsym(handle, "clEnqueueMigrateMemObjects");
	rtlib_ocl.enqueueNDRangeKernel = (enqueueNDRangeKernel_t) dlsym(handle, "clEnqueueNDRangeKernel");
	rtlib_ocl.enqueueTask = (enqueueTask_t) dlsym(handle, "clEnqueueTask");
	rtlib_ocl.enqueueNativeKernel = (enqueueNativeKernel_t) dlsym(handle, "clEnqueueNativeKernel");
	rtlib_ocl.enqueueMarkerWithWaitList = (enqueueMarkerWithWaitList_t) dlsym(handle, "clEnqueueMarkerWithWaitList");
	rtlib_ocl.enqueueBarrierWithWaitList = (enqueueBarrierWithWaitList_t) dlsym(handle, "clEnqueueBarrierWithWaitList");
	rtlib_ocl.flush = (flush_t) dlsym(handle, "clFlush");
	rtlib_ocl.finish = (finish_t) dlsym(handle, "clFinish");

	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_READ_BUFFER, "clEnqueueReadBuffer"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_READ_BUFFER_RECT, "clEnqueueReadBufferRect"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_WRITE_BUFFER, "clEnqueueWriteBuffer"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_WRITE_BUFFER_RECT, "clEnqueueWriteBufferRect"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_COPY_BUFFER, "clEnqueueCopyBuffer"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_COPY_BUFFER_RECT, "clEnqueueCopyBufferRect"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_READ_IMAGE, "clEnqueueReadImage"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_WRITE_IMAGE, "clEnqueueWriteImage"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_COPY_IMAGE, "clEnqueueCopyImage"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_COPY_IMAGE_TO_BUFFER, "clEnqueueCopyImageToBuffer"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_COPY_BUFFER_TO_IMAGE, "clEnqueueCopyBufferToImage"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_MAP_BUFFER, "clEnqueueMapBuffer"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_MAP_IMAGE, "clEnqueueMapImage"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_NDRANGE_KERNEL, "clEnqueueNDRangeKernel"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_TASK, "clEnqueueTask"));
	ocl_cmd_str.insert(CmdStrPair_t(CL_COMMAND_NATIVE_KERNEL, "clEnqueueNativeKernel"));
}

void rtlib_ocl_coll_event(cl_command_queue command_queue, cl_event *event) {
	std::map<cl_command_queue, QueueProfPtr_t>::iterator it;
	clRetainEvent(*event);
	it = ocl_queues_prof.find(command_queue);
	if (it == ocl_queues_prof.end())
		ocl_queues_prof.insert(
			std::pair<cl_command_queue, QueueProfPtr_t>(
				command_queue,
				QueueProfPtr_t(new RTLIB_OCL_QueueProf))
		);

	ocl_queues_prof[command_queue]->events.push_back(event);
}

void rtlib_ocl_prof_clean() {
	ocl_queues_prof.clear();
}

void rtlib_ocl_prof_run() {
	cl_command_type cmd_type = 0;
	cl_int status;
	double p_value = 0.0;
	std::vector<cl_event>::iterator it_v;
	std::map<cl_command_queue, QueueProfPtr_t>::iterator it;
	fprintf(stderr, FE("------>>>>>>>>>>>>>>>>>>> Profiling \n"));
	for (it = ocl_queues_prof.begin(); it != ocl_queues_prof.end(); it++) {
		clFinish(it->first);
		QueueProfPtr_t stPtr = it->second;
		status = clWaitForEvents(stPtr->events.size(), stPtr->events.data());
		if (status != CL_SUCCESS) {
			fprintf(stderr, FE("OCL: Error [%d] in clWaitForEvents\n"), status);
		}
		for (it_v = stPtr->events.begin(); it_v < stPtr->events.end(); it_v++){
			get_command_prof_info(*it_v, cmd_type, p_value);
			acc_command_stats(stPtr, cmd_type, p_value);
			//#ifdef abilitato
			dump_command_prof_info(cmd_type, p_value);
		}
	}
}

void acc_command_stats(QueueProfPtr_t stPtr, cl_command_type cmd_type, double p_value) {
	std::map<cl_command_type, AccProfPtr_t>::iterator it_ct;
	it_ct = stPtr->cmd_prof.find(cmd_type);
	if (it_ct == stPtr->cmd_prof.end())
		stPtr->cmd_prof.insert(
			CmdProfPair_t(cmd_type, AccProfPtr_t(new AccProf_t)));
	AccProf_t & p_acc(*(stPtr->cmd_prof[cmd_type].get()));
	p_acc(p_value);
}

void dump_command_prof_info(cl_command_type cmd_type, double p_value) {
	FILE *dump_file;
	char buffer [100];
	snprintf(buffer, 100, "%s.dat", ocl_cmd_str[cmd_type].c_str());
	dump_file = fopen(buffer, "a");
	if (dump_file) {
		fprintf(dump_file, "%d %f\n", cmd_type, p_value*1e-06);
	}
	fclose(dump_file);
}

void get_command_prof_info(cl_event event, cl_command_type & cmd_type, double & p_value) {
	size_t return_bytes;
	cl_int status;
	cl_ulong ev_start_time = (cl_ulong)0;
	cl_ulong ev_end_time   = (cl_ulong)0;
	clGetEventInfo(event, CL_EVENT_COMMAND_TYPE, sizeof(cl_command_type), &cmd_type, NULL);
	status = clGetEventProfilingInfo(
		event,
		CL_PROFILING_COMMAND_QUEUED,
		sizeof(cl_ulong),
		&ev_start_time,
		&return_bytes);
	status = clGetEventProfilingInfo(
		event,
		CL_PROFILING_COMMAND_END,
		sizeof(cl_ulong),
		&ev_end_time,
		&return_bytes);
	p_value = (double)(ev_end_time - ev_start_time);
}

#ifdef  __cplusplus
}
#endif
