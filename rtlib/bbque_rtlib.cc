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

#include "bbque/rtlib.h"

#include "bbque/version.h"
#include "bbque/utils/timer.h"
#include "bbque/utils/utility.h"

#include "bbque/rtlib/bbque_ocl.h"
#include "bbque/rtlib/bbque_rpc.h"

#ifdef CONFIG_TARGET_ANDROID
# include <stdint.h>
#else
# include <cstdint>
#endif

namespace bb = bbque;
namespace bu = bbque::utils;
namespace br = bbque::rtlib;

// Setup logging
#undef  BBQUE_LOG_MODULE
#define BBQUE_LOG_MODULE "rtl"

/**
 * The global timer, this can be used to get the time since the RTLib has been
 * initialized */
bu::Timer bbque_tmr(true);

/**
 * A pointer to the Barbeque RPC communication channel
 */
static br::BbqueRPC *rpc = NULL;

/**
 * The collection of RTLib services accessible from applications.
 */
static RTLIB_Services_t rtlib_services;

/**
 * The collection of RTLib wrapped OpenCL functions.
 */
RTLIB_OpenCL_t rtlib_ocl;

static RTLIB_ExecutionContextHandler_t rtlib_register(const char *name,
		const RTLIB_ExecutionContextParams_t *params) {
	return rpc->Register(name, params);
}

static void rtlib_unregister(
		const RTLIB_ExecutionContextHandler_t ech) {
	return rpc->Unregister(ech);
}

static RTLIB_ExitCode_t rtlib_enable(
		const RTLIB_ExecutionContextHandler_t ech) {
	return rpc->Enable(ech);
}

static RTLIB_ExitCode_t rtlib_disable(
		const RTLIB_ExecutionContextHandler_t ech) {
	return rpc->Disable(ech);
}

static RTLIB_ExitCode_t rtlib_getwm(
		const RTLIB_ExecutionContextHandler_t ech,
		RTLIB_WorkingModeParams_t *wm,
		RTLIB_SyncType_t st) {
	return rpc->GetWorkingMode(ech, wm, st);
}

static RTLIB_ExitCode_t rtlib_set(
		RTLIB_ExecutionContextHandler_t ech,
		RTLIB_Constraint_t *constraints, uint8_t count) {
	return rpc->Set(ech, constraints, count);
}

static RTLIB_ExitCode_t rtlib_clear(
		RTLIB_ExecutionContextHandler_t ech) {
	return rpc->Clear(ech);
}

static RTLIB_ExitCode_t rtlib_ggap(
		RTLIB_ExecutionContextHandler_t ech,
		uint8_t gap) {
	return rpc->GGap(ech, gap);
}

/*******************************************************************************
 *    Utility Functions
 ******************************************************************************/

static const char *rtlib_utils_getchuid() {
	return rpc->GetChUid();
}

static AppUid_t rtlib_utils_getuid(
		RTLIB_ExecutionContextHandler_t ech) {
	return rpc->GetUid(ech);
}

/*******************************************************************************
 *    Cycles Per Second (CPS) Control Support
 ******************************************************************************/

static RTLIB_ExitCode_t rtlib_cps_set(
		RTLIB_ExecutionContextHandler_t ech,
		float cps) {
	return rpc->SetCPS(ech, cps);
}

static RTLIB_ExitCode_t rtlib_cps_set_ctime_us(
		RTLIB_ExecutionContextHandler_t ech,
		uint32_t us) {
	return rpc->SetCTimeUs(ech, us);
}

/*******************************************************************************
 *    Performance Monitoring Support
 ******************************************************************************/

static void rtlib_notify_setup(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifySetup(ech);
}

static void rtlib_notify_init(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyInit(ech);
}

static void rtlib_notify_exit(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyExit(ech);
}

static void rtlib_notify_pre_configure(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPreConfigure(ech);
}

static void rtlib_notify_post_configure(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPostConfigure(ech);
}

static void rtlib_notify_pre_run(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPreRun(ech);
}

static void rtlib_notify_post_run(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPostRun(ech);
}

static void rtlib_notify_pre_monitor(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPreMonitor(ech);
}

static void rtlib_notify_post_monitor(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPostMonitor(ech);
}

static void rtlib_notify_pre_suspend(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPreSuspend(ech);
}

static void rtlib_notify_post_suspend(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPostSuspend(ech);
}

static void rtlib_notify_pre_resume(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPreResume(ech);
}

static void rtlib_notify_post_resume(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyPostResume(ech);
}

static void rtlib_notify_release(
		RTLIB_ExecutionContextHandler_t ech) {
	rpc->NotifyRelease(ech);
}

static const char *rtlib_app_name;
static uint8_t rtlib_initialized = 0;

#include "bbque_errors.cc"

RTLIB_ExitCode_t RTLIB_Init(const char *name, RTLIB_Services_t **rtlib) {
	RTLIB_ExitCode_t result;
	(*rtlib) = NULL;

	// Checking error string array consistency
	static_assert(ARRAY_SIZE(RTLIB_errorStr) == RTLIB_EXIT_CODE_COUNT,
			"RTLIB error strings not matching errors count");

	assert(rtlib_initialized==0);

	// Welcome screen
	fprintf(stderr, FI("Barbeque RTLIB (ver. %s)\n"), g_git_version);
	fprintf(stderr, FI("Built: " __DATE__  " " __TIME__ "\n"));

	// Data structure initialization
	rtlib_services.version.major = RTLIB_VERSION_MAJOR;
	rtlib_services.version.minor = RTLIB_VERSION_MINOR;

	rtlib_services.Register = rtlib_register;
	rtlib_services.Enable = rtlib_enable;
	rtlib_services.GetWorkingMode = rtlib_getwm;
	rtlib_services.SetConstraints = rtlib_set;
	rtlib_services.ClearConstraints = rtlib_clear;
	rtlib_services.SetGoalGap = rtlib_ggap;
	rtlib_services.Disable = rtlib_disable;
	rtlib_services.Unregister = rtlib_unregister;

	// Utility functions interface
	rtlib_services.Utils.GetChUid = rtlib_utils_getchuid;
	rtlib_services.Utils.GetUid = rtlib_utils_getuid;

	// Cycles Time Control interface
	rtlib_services.CPS.Set = rtlib_cps_set;
	rtlib_services.CPS.SetCTimeUs = rtlib_cps_set_ctime_us;

	// Performance monitoring notifiers
	rtlib_services.Notify.Setup = rtlib_notify_setup;
	rtlib_services.Notify.Init = rtlib_notify_init;
	rtlib_services.Notify.Exit = rtlib_notify_exit;
	rtlib_services.Notify.PreConfigure = rtlib_notify_pre_configure;
	rtlib_services.Notify.PostConfigure = rtlib_notify_post_configure;
	rtlib_services.Notify.PreRun = rtlib_notify_pre_run;
	rtlib_services.Notify.PostRun = rtlib_notify_post_run;
	rtlib_services.Notify.PreMonitor = rtlib_notify_pre_monitor;
	rtlib_services.Notify.PostMonitor = rtlib_notify_post_monitor;
	rtlib_services.Notify.PreSuspend = rtlib_notify_pre_suspend;
	rtlib_services.Notify.PostSuspend = rtlib_notify_post_suspend;
	rtlib_services.Notify.PreResume = rtlib_notify_pre_resume;
	rtlib_services.Notify.PostResume = rtlib_notify_post_resume;
	rtlib_services.Notify.Release = rtlib_notify_release;

	// Initialize OpenCL wrappers
	rtlib_ocl.getPlatformIDs =
		(cl_int (*) (cl_uint, cl_platform_id *, cl_uint *))
			dlsym(RTLD_NEXT, "clGetPlatformIDs");
	rtlib_ocl.getPlatformInfo =
		(cl_int (*) (cl_platform_id, cl_platform_info, size_t, void *, size_t *))
			dlsym(RTLD_NEXT, "clGetPlatformInfo");
	rtlib_ocl.getDeviceIDs =
		(cl_int (*) (cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *))
			dlsym(RTLD_NEXT, "clGetDeviceIDs");
	rtlib_ocl.getDeviceInfo =
		(cl_int (*) (cl_device_id, cl_device_info, size_t, void *, size_t *))
			dlsym(RTLD_NEXT, "clGetDeviceInfo");
	rtlib_ocl.createSubDevices =
		(cl_int (*) (cl_device_id, const cl_device_partition_property *, cl_uint, cl_device_id *, cl_uint *))
			dlsym(RTLD_NEXT, "clCreateSubDevices");
	rtlib_ocl.retainDevice =
		(cl_int (*) (cl_device_id)) dlsym(RTLD_NEXT, "clRetainDevice");
	rtlib_ocl.releaseDevice =
		(cl_int (*) (cl_device_id)) dlsym(RTLD_NEXT, "clReleaseDevice");
	rtlib_ocl.createContext =
		(cl_context (*) (const cl_context_properties *, cl_uint, const cl_device_id *, void (CL_CALLBACK *)(const char *, const void *, size_t, void *), void *, cl_int *))
			dlsym(RTLD_NEXT, "clCreateContext");
	rtlib_ocl.createContextFromType =
		(cl_context (*) (const cl_context_properties *, cl_device_type, void (CL_CALLBACK *)(const char *, const void *, size_t, void *), void *, cl_int *))
			dlsym(RTLD_NEXT, "clCreateContextFromType");
	rtlib_ocl.retainContext =
		(cl_int (*) (cl_context)) dlsym(RTLD_NEXT, "clRetainContext");
	rtlib_ocl.releaseContext =
		(cl_int (*) (cl_context)) dlsym(RTLD_NEXT, "clReleaseContext");
	rtlib_ocl.getContextInfo =
		(cl_int (*) (cl_context, cl_context_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetContextInfo");
	rtlib_ocl.createCommandQueue =
		(cl_command_queue (*) (cl_context, cl_device_id, cl_command_queue_properties, cl_int *))
		dlsym(RTLD_NEXT, "clCreateCommandQueue");
	rtlib_ocl.retainCommandQueue =
		(cl_int (*) (cl_command_queue)) dlsym(RTLD_NEXT, "clRetainCommandQueue");
	rtlib_ocl.releaseCommandQueue =
		(cl_int (*) (cl_command_queue)) dlsym(RTLD_NEXT, "clReleaseCommandQueue");
	rtlib_ocl.getCommandQueueInfo =
		(cl_int (*) (cl_command_queue, cl_command_queue_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetCommandQueueInfo");
	rtlib_ocl.createBuffer =
		(cl_mem (*) (cl_context, cl_mem_flags, size_t, void *, cl_int *))
		dlsym(RTLD_NEXT, "clCreateBuffer");
	rtlib_ocl.createSubBuffer =
		(cl_mem (*)(cl_mem, cl_mem_flags, cl_buffer_create_type, const void *, cl_int *))
		dlsym(RTLD_NEXT, "clCreateSubBuffer");
	rtlib_ocl.createImage =
		(cl_mem (*)(cl_context, cl_mem_flags, const cl_image_format *, const cl_image_desc *, void *, cl_int *))
		dlsym(RTLD_NEXT, "clCreateImage");
	rtlib_ocl.retainMemObject =
		(cl_int (*)(cl_mem)) dlsym(RTLD_NEXT, "clRetainMemObject");
	rtlib_ocl.releaseMemObject =
		(cl_int (*)(cl_mem)) dlsym(RTLD_NEXT, "clReleaseMemObject");
	rtlib_ocl.getSupportedImageFormats =
		(cl_int (*)(cl_context, cl_mem_flags, cl_mem_object_type, cl_uint, cl_image_format *, cl_uint *))
		dlsym(RTLD_NEXT, "clGetSupportedImageFormats");
	rtlib_ocl.getMemObjectInfo =
		(cl_int (*)(cl_mem, cl_mem_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetMemObjectInfo");
	rtlib_ocl.getImageInfo =
		(cl_int (*)(cl_mem, cl_image_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetImageInfo");
	rtlib_ocl.setMemObjectDestructorCallback =
		(cl_int (*)(cl_mem, void (CL_CALLBACK *)(cl_mem, void*), void *))
		dlsym(RTLD_NEXT, "clSetMemObjectDestructorCallback");
	rtlib_ocl.createSampler =
		(cl_sampler (*)(cl_context, cl_bool, cl_addressing_mode, cl_filter_mode, cl_int *))
		dlsym(RTLD_NEXT, "clCreateSampler");
	rtlib_ocl.retainSampler =
		(cl_int (*)(cl_sampler))
		dlsym(RTLD_NEXT, "clRetainSampler");
	rtlib_ocl.releaseSampler =
		(cl_int (*)(cl_sampler))
		dlsym(RTLD_NEXT, "clReleaseSampler");
	rtlib_ocl.getSamplerInfo =
		(cl_int (*)(cl_sampler, cl_sampler_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetSamplerInfo");
	rtlib_ocl.createProgramWithSource =
		(cl_program (*)(cl_context, cl_uint, const char **, const size_t *, cl_int *))
		dlsym(RTLD_NEXT, "clCreateProgramWithSource");
	rtlib_ocl.createProgramWithBinary =
		(cl_program (*)(cl_context, cl_uint, const cl_device_id *, const size_t *, const unsigned char **, cl_int *, cl_int *))
		dlsym(RTLD_NEXT, "clCreateProgramWithBinary");
	rtlib_ocl.createProgramWithBuiltInKernels =
		(cl_program (*)(cl_context, cl_uint, const cl_device_id *, const char *, cl_int *))
		dlsym(RTLD_NEXT, "clCreateProgramWithBuiltInKernels");
	rtlib_ocl.retainProgram =
		(cl_int (*)(cl_program))
		dlsym(RTLD_NEXT, "clRetainProgram");
	rtlib_ocl.releaseProgram =
		(cl_int (*)(cl_program))
		dlsym(RTLD_NEXT, "clReleaseProgram");
	rtlib_ocl.buildProgram =
		(cl_int (*)(cl_program, cl_uint, const cl_device_id *, const char *, void (CL_CALLBACK *)(cl_program, void *), void *))
		dlsym(RTLD_NEXT, "clBuildProgram");
	rtlib_ocl.compileProgram =
		(cl_int (*)(cl_program, cl_uint, const cl_device_id *, const char *, cl_uint, const cl_program *, const char **, void (CL_CALLBACK *)(cl_program, void *), void *))
		dlsym(RTLD_NEXT, "clCompileProgram");
	rtlib_ocl.linkProgram =
		(cl_program (*)(cl_context, cl_uint, const cl_device_id *, const char *, cl_uint, const cl_program *, void (CL_CALLBACK *)(cl_program, void *), void *, cl_int *))
		dlsym(RTLD_NEXT, "clLinkProgram");
	rtlib_ocl.unloadPlatformCompiler =
		(cl_int (*)(cl_platform_id))
		dlsym(RTLD_NEXT, "clUnloadPlatformCompiler");
	rtlib_ocl.getProgramInfo =
		(cl_int (*)(cl_program, cl_program_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetProgramInfo");
	rtlib_ocl.getProgramBuildInfo =
		(cl_int (*)(cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetProgramBuildInfo");
	rtlib_ocl.createKernel =
		(cl_kernel (*)(cl_program, const char *, cl_int *))
		dlsym(RTLD_NEXT, "clCreateKernel");
	rtlib_ocl.createKernelsInProgram =
		(cl_int (*)(cl_program, cl_uint, cl_kernel *, cl_uint *))
		dlsym(RTLD_NEXT, "clCreateKernelsInProgram");
	rtlib_ocl.retainKernel =
		(cl_int (*)(cl_kernel))
		dlsym(RTLD_NEXT, "clRetainKernel");
	rtlib_ocl.releaseKernel =
		(cl_int (*)(cl_kernel))
		dlsym(RTLD_NEXT, "clReleaseKernel");
	rtlib_ocl.setKernelArg =
		(cl_int (*)(cl_kernel, cl_uint, size_t, const void *))
		dlsym(RTLD_NEXT, "clSetKernelArg");
	rtlib_ocl.getKernelInfo =
		(cl_int (*)(cl_kernel, cl_kernel_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetKernelInfo");
	rtlib_ocl.getKernelArgInfo =
		(cl_int (*)(cl_kernel, cl_uint, cl_kernel_arg_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetKernelArgInfo");
	rtlib_ocl.getKernelWorkGroupInfo =
		(cl_int (*)(cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetKernelWorkGroupInfo");
	rtlib_ocl.waitForEvents =
		(cl_int (*)(cl_uint, const cl_event *))
		dlsym(RTLD_NEXT, "clWaitForEvents");
	rtlib_ocl.getEventInfo =
		(cl_int (*)(cl_event, cl_event_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetEventInfo");
	rtlib_ocl.createUserEvent =
		(cl_event (*)(cl_context, cl_int *))
		dlsym(RTLD_NEXT, "clCreateUserEvent");
	rtlib_ocl.retainEvent =
		(cl_int (*)(cl_event))
		dlsym(RTLD_NEXT, "clRetainEvent");
	rtlib_ocl.releaseEvent =
		(cl_int (*)(cl_event))
		dlsym(RTLD_NEXT, "clReleaseEvent");
	rtlib_ocl.setUserEventStatus =
		(cl_int (*)(cl_event, cl_int))
		dlsym(RTLD_NEXT, "clSetUserEventStatus");
	rtlib_ocl.setEventCallback =
		(cl_int (*)(cl_event, cl_int, void (CL_CALLBACK *)(cl_event, cl_int, void *),
		void *))
		dlsym(RTLD_NEXT, "clSetEventCallback");
	rtlib_ocl.getEventProfilingInfo =
		(cl_int (*)(cl_event, cl_profiling_info, size_t, void *, size_t *))
		dlsym(RTLD_NEXT, "clGetEventProfilingInfo");

	// Building a communication channel
	rpc = br::BbqueRPC::GetInstance();
	if (!rpc) {
		fprintf(stderr, FE("RPC communication channel build FAILED\n"));
		return RTLIB_BBQUE_CHANNEL_SETUP_FAILED;
	}

	// Initializing the RPC communication channel
	result = rpc->Init(name);
	if (result!=RTLIB_OK) {
		fprintf(stderr, FE("RPC communication channel "
					"initialization FAILED\n"));
		return RTLIB_BBQUE_UNREACHABLE;
	}

	// Marking library as intialized
	rtlib_initialized = 1;
	rtlib_app_name = name;

	(*rtlib) = &rtlib_services;
	return RTLIB_OK;
}

__attribute__((destructor))
static void RTLIB_Exit(void) {

	DB(fprintf(stderr, FD("Barbeque RTLIB, Cleanup and release\n")));

	if (!rtlib_initialized)
		return;

	// Close the RPC FIFO channel thus releasin all BBQUE resource used by
	// this application
	assert(rpc);

	// Ensure all the EXCs are unregistered
	rpc->UnregisterAll();
	delete rpc;

	if (0) RTLIB_Exit(); // Fix compilation warning

}


