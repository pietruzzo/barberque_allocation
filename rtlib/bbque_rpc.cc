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

#include "bbque/config.h"
#include "bbque/rtlib/bbque_rpc.h"
#include "bbque/rtlib/rpc_fifo_client.h"
#include "bbque/rtlib/rpc_unmanaged_client.h"
#include "bbque/app/application.h"
#include "bbque/utils/logging/console_logger.h"

#include <cstdio>
#include <sys/stat.h>

#ifdef CONFIG_BBQUE_OPENCL
  #include "bbque/rtlib/bbque_ocl.h"
#endif

// Setup logging
#undef  BBQUE_LOG_MODULE
#define BBQUE_LOG_MODULE "rpc"

namespace ba = bbque::app;
namespace bu = bbque::utils;

namespace bbque { namespace rtlib {

std::unique_ptr<bu::Logger> BbqueRPC::logger;

BbqueRPC * BbqueRPC::GetInstance() {
	static BbqueRPC * instance = NULL;

	if (instance)
		return instance;

	// Get a Logger module
	logger = bu::Logger::GetLogger(BBQUE_LOG_MODULE);

	// Parse environment configuration
	ParseOptions();

#ifdef CONFIG_BBQUE_RTLIB_UNMANAGED_SUPPORT
	if (envUnmanaged) {
		logger->Warn("Running in UNMANAGED MODE");
		instance = new BbqueRPC_UNMANAGED_Client();
		goto channel_done;
	}
#endif

#ifdef CONFIG_BBQUE_RPC_FIFO
	logger->Debug("Using FIFO RPC channel");
	instance = new BbqueRPC_FIFO_Client();
#else
#error RPC Channel NOT defined
#endif // CONFIG_BBQUE_RPC_FIFO

	// Compilation warning fix
	goto channel_done;
channel_done:

	return instance;
}

BbqueRPC::~BbqueRPC(void) {
	logger = bu::ConsoleLogger::GetInstance(BBQUE_LOG_MODULE);
	logger->Debug("BbqueRPC dtor");

	// Clean-up all the registered EXCs
	exc_map.clear();
}

bool BbqueRPC::envPerfCount = false;
bool BbqueRPC::envGlobal = false;
bool BbqueRPC::envOverheads = false;
int  BbqueRPC::envDetailedRun = 0;
int  BbqueRPC::envRawPerfCount = 0;
bool BbqueRPC::envNoKernel = false;
bool BbqueRPC::envCsvOutput = false;
bool BbqueRPC::envMOSTOutput = false;
#ifdef CONFIG_BBQUE_OPENCL
bool BbqueRPC::envOCLProf = false;
int  BbqueRPC::envOCLProfLevel = 0;
#endif //CONFIG_BBQUE_OPENCL
char BbqueRPC::envMetricsTag[BBQUE_RTLIB_OPTS_TAG_MAX+2] = "";
bool BbqueRPC::envBigNum = false;
bool BbqueRPC::envUnmanaged = false;
int  BbqueRPC::envUnmanagedAWM = 0;
const char *BbqueRPC::envCsvSep = " ";

// Select if statistics should be dumped on a file
bool BbqueRPC::envFileOutput = false;
// This is the file handler used for statistics dumping
static FILE *outfd = stderr;


RTLIB_ExitCode_t BbqueRPC::ParseOptions() {
	const char *env;
	char buff[100];
	char *opt;
#ifdef CONFIG_BBQUE_RTLIB_PERF_SUPPORT
	char * raw_buff;
	int8_t idx  = 0;
	size_t nchr = 0;
#endif

	logger->Debug("Parsing environment options...");

	// Look-for the expected RTLIB configuration variable
	env = getenv("BBQUE_RTLIB_OPTS");
	if (!env)
		return RTLIB_OK;

	logger->Debug("BBQUE_RTLIB_OPTS: [%s]", env);

	// Tokenize configuration options
	strncpy(buff, env, sizeof(buff));
	opt = strtok(buff, ":");

	// Parsing all options (only single char opts are supported)
	while (opt) {

		logger->Debug("OPT: %s", opt);

		switch (opt[0]) {
		case 'G':
			// Enabling Global statistics collection
			envGlobal = true;
			break;
		case 'K':
			// Disable Kernel and Hipervisor from collected statistics
			envNoKernel = true;
			break;
		case 'M':
			// Enabling MOST output
			envMOSTOutput = true;
			// Check if a TAG has been specified
			if (opt[1]) {
				snprintf(envMetricsTag,
						BBQUE_RTLIB_OPTS_TAG_MAX,
						"%s:", opt+1);
			}
			logger->Info("Enabling MOST output [tag: %s]",
					envMetricsTag[0] ? envMetricsTag : "-");
			break;
		case 'O':
			// Collect statistics on RTLIB overheads
			envOverheads = true;
			break;
		case 'U':
			// Enable "unmanaged" mode with the specified AWM
			envUnmanaged = true;
			if (*(opt+1))
				sscanf(opt+1, "%d", &envUnmanagedAWM);
			logger->Warn("Enabling UNMANAGED mode");
			break;
		case 'b':
			// Enabling "big numbers" notations
			envBigNum = true;
			break;
		case 'c':
			// Enabling CSV output
			envCsvOutput = true;
			break;
		case 'f':
			// Enabling File output
			envFileOutput = true;
			logger->Notice("Enabling statistics dump on FILE");
			break;
		case 'p':
			// Enabling perf...
			envPerfCount = BBQUE_RTLIB_PERF_ENABLE;
			// ... with the specified verbosity level
			sscanf(opt+1, "%d", &envDetailedRun);
			if (envPerfCount) {
				logger->Info("Enabling Perf Counters [verbosity: %d]", envDetailedRun);
			} else {
				logger->Error("WARN: Perf Counters NOT available");
			}
			break;
#ifdef CONFIG_BBQUE_RTLIB_PERF_SUPPORT
		case 'r':
			// Enabling perf...
			envPerfCount = BBQUE_RTLIB_PERF_ENABLE;

			// # of RAW perf counters
			sscanf(opt+1, "%d", &envRawPerfCount);
			if (envRawPerfCount > 0) {
				logger->Info("Enabling %d RAW Perf Counters", envRawPerfCount);
			} else {
				logger->Warn("Expected RAW Perf Counters");
				break;
			}

			// Get the first raw performance counter
			raw_buff = opt+3;
			nchr = strcspn(raw_buff, ",");
			raw_buff[nchr] = '\0';

			// Insert the events into the array
			while (raw_buff[0] != '\0') {
				idx = InsertRAWPerfCounter(raw_buff);
				if (idx == envRawPerfCount)
					break;

				// Get the next raw performance counter
				raw_buff += nchr+1;
				nchr = strcspn(raw_buff, ",");
				raw_buff[nchr] = '\0';
			}
			envRawPerfCount = idx;
			break;
#endif //CONFIG_BBQUE_RTLIB_PERF_SUPPORT

#ifdef CONFIG_BBQUE_OPENCL
		case 'o':
			// Enabling OpenCL Profiling Output on file
			envOCLProf = true;
			sscanf(opt+1, "%d", &envOCLProfLevel);
			fprintf(stderr, "Enabling OpenCL profiling [verbosity: %d]\n", envOCLProfLevel);
			break;
#endif //CONFIG_BBQUE_OPENCL

		case 's':
			// Setting CSV separator
			if (opt[1])
				envCsvSep = opt+1;
			break;
		}

		// Get next option
		opt = strtok(NULL, ":");
	}

	return RTLIB_OK;
}

// Thereafter methods have an instance, thus we could specialize logging
#undef  BBQUE_LOG_UID
#define BBQUE_LOG_UID GetChUid()

RTLIB_ExitCode_t BbqueRPC::Init(const char *name) {
	RTLIB_ExitCode_t exitCode;

	if (initialized) {
		logger->Warn("RTLIB already initialized for app [%d:%s]",
				appTrdPid, name);
		return RTLIB_OK;
	}

	appName = name;

	// Getting application PID
	appTrdPid = gettid();

	logger->Debug("Initializing app [%d:%s]", appTrdPid, name);

	exitCode = _Init(name);
	if (exitCode != RTLIB_OK) {
		logger->Error("Initialization FAILED");
		return exitCode;
	}

	initialized = true;

	logger->Debug("Initialation DONE");
	return RTLIB_OK;

}

uint8_t BbqueRPC::GetNextExcID() {
	static uint8_t exc_id = 0;
	excMap_t::iterator it = exc_map.find(exc_id);

	// Ensuring unicity of the Execution Context ID
	while (it != exc_map.end()) {
		exc_id++;
		it = exc_map.find(exc_id);
	}

	return exc_id;
}

RTLIB_ExecutionContextHandler_t BbqueRPC::Register(
		const char* name,
		const RTLIB_ExecutionContextParams_t* params) {
	RTLIB_ExitCode_t result;
	excMap_t::iterator it(exc_map.begin());
	excMap_t::iterator end(exc_map.end());
	pregExCtx_t prec;

	assert(initialized);
	assert(name && params);

	logger->Info("Registering EXC [%s]...", name);

	if (!initialized) {
		logger->Error("Registering EXC [%s] FAILED "
					"(Error: RTLIB not initialized)", name);
		return NULL;
	}

	// Ensuring the execution context has not been already registered
	for( ; it != end; ++it) {
		prec = (*it).second;
		if (prec->name == name) {
			logger->Error("Registering EXC [%s] FAILED "
				"(Error: EXC already registered)", name);
			assert(prec->name != name);
			return NULL;
		}
	}

	// Build a new registered EXC
	prec = pregExCtx_t(new RegisteredExecutionContext_t(name, GetNextExcID()));
	memcpy((void*)&(prec->exc_params), (void*)params,
			sizeof(RTLIB_ExecutionContextParams_t));

	// Calling the Low-level registration
	result = _Register(prec);
	if (result != RTLIB_OK) {
		DB(logger->Error("Registering EXC [%s] FAILED "
			"(Error %d: %s)", name, result, RTLIB_ErrorStr(result)));
		return NULL;
	}

	// Save the registered execution context
	exc_map.insert(excMapEntry_t(prec->exc_id, prec));

	// Mark the EXC as Registered
	setRegistered(prec);

	return (RTLIB_ExecutionContextHandler_t)&(prec->exc_params);
}

BbqueRPC::pregExCtx_t BbqueRPC::getRegistered(
		const RTLIB_ExecutionContextHandler_t ech) {
	excMap_t::iterator it(exc_map.begin());
	excMap_t::iterator end(exc_map.end());
	pregExCtx_t prec;

	assert(ech);

	// Checking for library initialization
	if (!initialized) {
		logger->Error("EXC [%p] lookup FAILED "
			"(Error: RTLIB not initialized)", (void*)ech);
		assert(initialized);
		return pregExCtx_t();
	}

	// Ensuring the execution context has been registered
	for( ; it != end; ++it) {
		prec = (*it).second;
		if ((void*)ech == (void*)&prec->exc_params)
			break;
	}
	if (it == end) {
		logger->Error("EXC [%p] lookup FAILED "
			"(Error: EXC not registered)", (void*)ech);
		assert(it != end);
		return pregExCtx_t();
	}

	return prec;
}

BbqueRPC::pregExCtx_t BbqueRPC::getRegistered(uint8_t exc_id) {
	excMap_t::iterator it(exc_map.find(exc_id));

	// Checking for library initialization
	if (!initialized) {
		logger->Error("EXC [%d] lookup FAILED "
			"(Error: RTLIB not initialized)", exc_id);
		assert(initialized);
		return pregExCtx_t();
	}

	if (it == exc_map.end()) {
		logger->Error("EXC [%d] lookup FAILED "
			"(Error: EXC not registered)", exc_id);
		assert(it != exc_map.end());
		return pregExCtx_t();
	}

	return (*it).second;
}

void BbqueRPC::Unregister(
		const RTLIB_ExecutionContextHandler_t ech) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Unregister EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return;
	}

	assert(isRegistered(prec) == true);

	// Dump (verbose) execution statistics
	DumpStats(prec, true);

	// Calling the low-level unregistration
	result = _Unregister(prec);
	if (result != RTLIB_OK) {
		DB(logger->Error("Unregister EXC [%p:%s] FAILED (Error %d: %s)",
				(void*)ech, prec->name.c_str(), result, RTLIB_ErrorStr(result)));
		return;
	}

	// Mark the EXC as Unregistered
	clearRegistered(prec);

}

void BbqueRPC::UnregisterAll() {
	RTLIB_ExitCode_t result;
	excMap_t::iterator it;
	pregExCtx_t prec;

	// Checking for library initialization
	if (!initialized) {
		logger->Error("EXCs cleanup FAILED (Error: RTLIB not initialized)");
		assert(initialized);
		return;
	}

	// Unregisterig all the registered EXCs
	it = exc_map.begin();
	for ( ; it != exc_map.end(); ++it) {
		prec = (*it).second;

		// Jumping already un-registered EXC
		if (!isRegistered(prec))
				continue;

		// Calling the low-level unregistration
		result = _Unregister(prec);
		if (result != RTLIB_OK) {
			DB(logger->Error("Unregister EXC [%s] FAILED (Error %d: %s)",
						prec->name.c_str(), result, RTLIB_ErrorStr(result)));
			return;
		}

		// Mark the EXC as Unregistered
		clearRegistered(prec);
	}

}


RTLIB_ExitCode_t BbqueRPC::Enable(
		const RTLIB_ExecutionContextHandler_t ech) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Enabling EXC [%p] FAILED "
				"(Error: EXC not registered)", (void*)ech);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	assert(isEnabled(prec) == false);

	// Calling the low-level enable function
	result = _Enable(prec);
	if (result != RTLIB_OK) {
		DB(logger->Error("Enabling EXC [%p:%s] FAILED (Error %d: %s)",
				(void*)ech, prec->name.c_str(), result, RTLIB_ErrorStr(result)));
		return RTLIB_EXC_ENABLE_FAILED;
	}

	// Mark the EXC as Enabled
	setEnabled(prec);
	clearAwmValid(prec);
	clearAwmAssigned(prec);

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::Disable(
		const RTLIB_ExecutionContextHandler_t ech) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Disabling EXC [%p] STOP "
				"(Error: EXC not registered)", (void*)ech);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	assert(isEnabled(prec) == true);

	// Calling the low-level disable function
	result = _Disable(prec);
	if (result != RTLIB_OK) {
		DB(logger->Error("Disabling EXC [%p:%s] FAILED (Error %d: %s)",
				(void*)ech, prec->name.c_str(), result, RTLIB_ErrorStr(result)));
		return RTLIB_EXC_DISABLE_FAILED;
	}

	// Mark the EXC as Enabled
	clearEnabled(prec);
	clearAwmValid(prec);
	clearAwmAssigned(prec);

	// Unlocking eventually waiting GetWorkingMode
	prec->cv.notify_one();

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::SetupStatistics(pregExCtx_t prec) {
	assert(prec);
	pAwmStats_t pstats(prec->stats[prec->awm_id]);

	// Check if this is a newly selected AWM
	if (!pstats) {
		logger->Debug("Setup stats for AWM [%d]", prec->awm_id);
		pstats = prec->stats[prec->awm_id] =
			pAwmStats_t(new AwmStats_t);

		// Setup Performance Counters (if required)
		if (PerfRegisteredEvents(prec)) {
			PerfSetupStats(prec, pstats);
		}
	}

	// Update usage count
	pstats->count++;

	// Configure current AWM stats
	prec->pAwmStats = pstats;

	return RTLIB_OK;
}

#define STATS_HEADER \
"# EXC    AWM   Uses Cycles   Total |      Min      Max |      Avg      Var"
#define STATS_AWM_SPLIT \
"#==================================+===================+=================="
#define STATS_CYCLE_SPLIT \
"#-------------------------+        +-------------------+------------------"
#define STATS_CONF_SPLIT \
"#-------------------------+--------+-------------------+------------------"

void BbqueRPC::DumpStatsHeader() {
	fprintf(outfd, STATS_HEADER "\n");
}

void BbqueRPC::DumpStatsConsole(pregExCtx_t prec, bool verbose) {
	AwmStatsMap_t::iterator it;
	pAwmStats_t pstats;
	uint8_t awm_id;

	uint32_t cycles_count;
	double cycle_min, cycle_max, cycle_avg, cycle_var;
	double monitor_min, monitor_max, monitor_avg, monitor_var;
	double config_min, config_max, config_avg, config_var;

	// Print RTLib stats for each AWM
	it = prec->stats.begin();
	for ( ; it != prec->stats.end(); ++it) {
		awm_id = (*it).first;
		pstats = (*it).second;

		// Ignoring empty statistics
		cycles_count = count(pstats->cycle_samples);
		if (!cycles_count)
			continue;

		// Cycles statistics extraction
		cycle_min = min(pstats->cycle_samples);
		cycle_max = max(pstats->cycle_samples);
		cycle_avg = mean(pstats->cycle_samples);
		cycle_var = variance(pstats->cycle_samples);

		if (verbose) {
			fprintf(outfd, STATS_AWM_SPLIT"\n");
			fprintf(outfd, "%8s %03d %6d %6d %7u | %8.3f %8.3f | %8.3f %8.3f\n",
				prec->name.c_str(), awm_id, pstats->count, cycles_count,
				pstats->time_processing, cycle_min, cycle_max, cycle_avg, cycle_var);
		} else {
			logger->Debug(STATS_AWM_SPLIT);
			logger->Debug("%8s %03d %6d %6d %7u | %8.3f %8.3f | %8.3f %8.3f",
				prec->name.c_str(), awm_id, pstats->count, cycles_count,
				pstats->time_processing, cycle_min, cycle_max, cycle_avg, cycle_var);
		}

		// Monitor statistics extraction
		monitor_min = min(pstats->monitor_samples);
		monitor_max = max(pstats->monitor_samples);
		monitor_avg = mean(pstats->monitor_samples);
		monitor_var = variance(pstats->monitor_samples);

		if (verbose) {
			fprintf(outfd, STATS_CYCLE_SPLIT "\n");
			fprintf(outfd, "%8s %03d %13s %7u | %8.3f %8.3f | %8.3f %8.3f\n",
				prec->name.c_str(), awm_id, "onRun",
				pstats->time_processing - pstats->time_monitoring,
				cycle_min - monitor_min,
				cycle_max - monitor_max,
				cycle_avg - monitor_avg,
				cycle_var - monitor_var);
			fprintf(outfd, "%8s %03d %13s %7u | %8.3f %8.3f | %8.3f %8.3f\n",
				prec->name.c_str(), awm_id, "onMonitor", pstats->time_monitoring,
				monitor_min, monitor_max, monitor_avg, monitor_var);
		} else {
			logger->Debug(STATS_AWM_SPLIT);
			logger->Debug("%8s %03d %13s %7u | %8.3f %8.3f | %8.3f %8.3f\n",
				prec->name.c_str(), awm_id, "onRun",
				pstats->time_processing - pstats->time_monitoring,
				cycle_min - monitor_min,
				cycle_max - monitor_max,
				cycle_avg - monitor_avg,
				cycle_var - monitor_var);
			logger->Debug("%8s %03d %13s %7u | %8.3f %8.3f | %8.3f %8.3f\n",
				prec->name.c_str(), awm_id, "onMonitor", pstats->time_monitoring,
				monitor_min, monitor_max, monitor_avg, monitor_var);
		}

		// Reconfiguration statistics extraction
		config_min = min(pstats->config_samples);
		config_max = max(pstats->config_samples);
		config_avg = mean(pstats->config_samples);
		config_var = variance(pstats->config_samples);

		if (verbose) {
			fprintf(outfd, STATS_CONF_SPLIT "\n");
			fprintf(outfd, "%8s %03d %13s %7u | %8.3f %8.3f | %8.3f %8.3f\n",
				prec->name.c_str(), awm_id, "onConfigure", pstats->time_configuring,
				config_min, config_max, config_avg, config_var);
		} else {
			logger->Debug(STATS_CONF_SPLIT);
			logger->Debug("%8s %03d %13s %7u | %8.3f %8.3f | %8.3f %8.3f\n",
				prec->name.c_str(), awm_id, "onConfigure", pstats->time_configuring,
				config_min, config_max, config_avg, config_var);
		}

	}

	if (!PerfRegisteredEvents(prec) || !verbose)
		return;

	// Print performance counters for each AWM
	it = prec->stats.begin();
	for ( ; it != prec->stats.end(); ++it) {
		awm_id = (*it).first;
		pstats = (*it).second;

		cycles_count = count(pstats->cycle_samples);
		fprintf(outfd, "\nPerf counters stats for '%s-%d' (%d cycles):\n\n",
				prec->name.c_str(), awm_id, cycles_count);
		PerfPrintStats(prec, pstats);
	}

}

static char _metricPrefix[64] = "";
static inline void _setMetricPrefix(const char *exc_name, uint8_t awm_id) {
	snprintf(_metricPrefix, 64, "%s:%02d", exc_name, awm_id);
}



#define DUMP_MOST_METRIC(CLASS, NAME, VALUE, FMT)	\
	fprintf(outfd, "@%s%s:%s:%s=" FMT "@\n",	\
			envMetricsTag,			\
			_metricPrefix,			\
			CLASS,				\
			NAME, 				\
			VALUE)

void BbqueRPC::DumpStatsMOST(pregExCtx_t prec) {
	AwmStatsMap_t::iterator it;
	pAwmStats_t pstats;
	uint8_t awm_id;

	uint32_t cycles_count, monitor_count, config_count;
	double cycle_min, cycle_max, cycle_avg, cycle_var;
	double monitor_min, monitor_max, monitor_avg, monitor_var;
	double config_min, config_max, config_avg, config_var;

	// Print RTLib stats for each AWM
	it = prec->stats.begin();
	for ( ; it != prec->stats.end(); ++it) {
		awm_id = (*it).first;
		pstats = (*it).second;

		// Ignoring empty statistics
		cycles_count = count(pstats->cycle_samples);
		if (!cycles_count)
			continue;

		_setMetricPrefix(prec->name.c_str(), awm_id);
		fprintf(outfd, ".:: MOST statistics for AWM [%s]:\n",
				_metricPrefix);

		// Cycles statistics extraction
		cycle_min = min(pstats->cycle_samples);
		cycle_max = max(pstats->cycle_samples);
		cycle_avg = mean(pstats->cycle_samples);
		cycle_var = variance(pstats->cycle_samples);

		DUMP_MOST_METRIC("perf", "cycles_cnt"   , cycles_count   , "%u");
		DUMP_MOST_METRIC("perf", "cycles_min_ms", cycle_min      , "%.3f");
		DUMP_MOST_METRIC("perf", "cycles_max_ms", cycle_max      , "%.3f");
		DUMP_MOST_METRIC("perf", "cycles_avg_ms", cycle_avg      , "%.3f");
		DUMP_MOST_METRIC("perf", "cycles_std_ms", sqrt(cycle_var), "%.3f");

		// Monitor statistics extraction
		monitor_count = count(pstats->monitor_samples);
		monitor_min = min(pstats->monitor_samples);
		monitor_max = max(pstats->monitor_samples);
		monitor_avg = mean(pstats->monitor_samples);
		monitor_var = variance(pstats->monitor_samples);

		DUMP_MOST_METRIC("perf", "monitor_cnt",    monitor_count    , "%u");
		DUMP_MOST_METRIC("perf", "monitor_min_ms", monitor_min      , "%.3f");
		DUMP_MOST_METRIC("perf", "monitor_max_ms", monitor_max      , "%.3f");
		DUMP_MOST_METRIC("perf", "monitor_avg_ms", monitor_avg      , "%.3f");
		DUMP_MOST_METRIC("perf", "monitor_std_ms", sqrt(monitor_var), "%.3f");

		// Reconfiguration statistics extraction
		config_count = count(pstats->config_samples);
		config_min = min(pstats->config_samples);
		config_max = max(pstats->config_samples);
		config_avg = mean(pstats->config_samples);
		config_var = variance(pstats->config_samples);

		DUMP_MOST_METRIC("perf", "configure_cnt",    config_count    , "%u");
		DUMP_MOST_METRIC("perf", "configure_min_ms", config_min      , "%.3f");
		DUMP_MOST_METRIC("perf", "configure_max_ms", config_max      , "%.3f");
		DUMP_MOST_METRIC("perf", "configure_avg_ms", config_avg      , "%.3f");
		DUMP_MOST_METRIC("perf", "configure_std_ms", sqrt(config_var), "%.3f");

		// Dump Performance Counters for this AWM
		PerfPrintStats(prec, pstats);

#ifdef CONFIG_BBQUE_OPENCL
		// Dump OpenCL profiling info for each AWM
		if (envOCLProf)
			OclPrintStats(pstats);
#endif //CONFIG_BBQUE_OPENCL

	}

	// Dump Memory Consumption report
	DumpMemoryReport(prec);

}

#ifdef CONFIG_BBQUE_RTLIB_CGROUPS_SUPPORT
RTLIB_ExitCode_t BbqueRPC::SetCGroupPath(pregExCtx_t prec) {
	uint8_t count = 0;
#define BBQUE_RPC_CGOUPS_PATH_MAX 128
	char cgMount[BBQUE_RPC_CGOUPS_PATH_MAX];
	char buff[256];
	char *pd, *ps;
	FILE *procfd;

	procfd = ::fopen("/proc/mounts", "r");
	if (!procfd) {
		logger->Error("Mounts read FAILED (Error %d: %s)",
					errno, strerror(errno));
		return RTLIB_EXC_CGROUP_NONE;
	}

	// Find CGroups mount point
	cgMount[0] = 0;
	for (;;) {
		if (!fgets(buff, 256, procfd))
			break;

		if (strncmp("bbque_cgroups ", buff, 14))
			continue;

		// copy mountpoint
		// NOTE: no spaces are allows on mountpoint
		ps = buff+14;
		pd = cgMount;
		while ((count < BBQUE_RPC_CGOUPS_PATH_MAX-1) && (*ps != ' ')) {
			*pd = *ps;
			++pd; ++ps;
			++count;
		}
		cgMount[count] = 0;
		break;

	}

	if (count == BBQUE_RPC_CGOUPS_PATH_MAX) {
		logger->Error("CGroups mount identification FAILED"
				"(Error: path longer than %d chars)",
				BBQUE_RPC_CGOUPS_PATH_MAX-1);
		return RTLIB_EXC_CGROUP_NONE;
	}

	if (!cgMount[0]) {
		logger->Error("CGroups mount identification FAILED");
		return RTLIB_EXC_CGROUP_NONE;
	}


	snprintf(buff, 256, "%s/bbque/%05d:%.6s:%02d",
			cgMount,
			chTrdPid,
			prec->name.c_str(),
			prec->exc_id);

	// Check CGroups access
	struct stat cgstat;
	if (stat(buff, &cgstat)) {
		logger->Error("CGroup [%s] access FAILED (Error %d: %s)",
				buff, errno, strerror(errno));
		return RTLIB_EXC_CGROUP_NONE;
	}

	pathCGroup = std::string(buff);
	logger->Debug("Application CGroup [%s] FOUND", pathCGroup.c_str());

	return RTLIB_OK;

}

void BbqueRPC::DumpMemoryReport(pregExCtx_t prec) {
	char metric[32];
	uint64_t value;
	char buff[256];
	FILE *memfd;
	(void)prec;

	// Check for CGroups being available
	if (!GetCGroupPath().length())
		return;

	// Open Memory statistics file
	snprintf(buff, 256, "%s/memory.stat", GetCGroupPath().c_str());
	memfd = ::fopen(buff, "r");
	if (!memfd) {
		logger->Error("Opening MEMORY stats FAILED (Error %d: %s)",
					errno, strerror(errno));
		return;
	}

	while (fgets(buff, 256, memfd)) {
		// DB(
		// 	// remove leading '\n'
		// 	buff[strlen(buff)-1] = 0;
		// 	logger->Debug("Memory Read [%s]", buff);
		// )
		sscanf(buff, "%32s %" PRIu64, metric, &value);
		DUMP_MOST_METRIC("memory", metric, value, "%" PRIu64);
	}

}
#else
RTLIB_ExitCode_t BbqueRPC::SetCGroupPath(pregExCtx_t prec) {
	(void)prec;
	return RTLIB_OK;
}
void BbqueRPC::DumpMemoryReport(pregExCtx_t prec) {
	(void)prec;
}
#endif // CONFIG_BBQUE_RTLIB_CGROUPS_SUPPPORT


void BbqueRPC::DumpStats(pregExCtx_t prec, bool verbose) {
	std::string outfile(BBQUE_PATH_VAR "/");

	// Statistics should be dumped only if:
	// - compiled in DEBUG mode, or
	// - VERBOSE execution is required
	// This check allows to avoid metrics computation in case the output
	// should not be generated.
	if (DB(false &&) !verbose)
		return;

	outfd = stderr;
	if (envFileOutput) {
		outfile += std::string("stats_") + chTrdUid + ":" + prec->name;
		outfd = fopen(outfile.c_str(), "w");
	}


	if (!envFileOutput)
		logger->Notice("Execution statistics:\n\n");

	// MOST statistics are dumped just at the end of the execution
	// (i.e. verbose mode)
	if (envMOSTOutput && verbose) {
		DumpStatsMOST(prec);
		goto exit_done;
	}

	DumpStatsHeader();
	DumpStatsConsole(prec, verbose);

#ifdef CONFIG_BBQUE_OPENCL
	// Dump OpenCL profiling info for each AWM
	if (envOCLProf)
		OclDumpStatsConsole(prec);
#endif //CONFIG_BBQUE_OPENCL

exit_done:
	if (envFileOutput) {
		fclose(outfd);
		logger->Warn("Execution statistics dumped on [%s]", outfile.c_str());
	}

}

void BbqueRPC::_SyncTimeEstimation(pregExCtx_t prec) {
	pAwmStats_t pstats(prec->pAwmStats);
	std::unique_lock<std::mutex> stats_ul(pstats->stats_mtx);
	// Use high resolution to avoid errors on next computations
	double last_cycle_ms;

	// TIMER: Get RUNNING
	last_cycle_ms = prec->exc_tmr.getElapsedTimeMs();

	logger->Debug("Last cycle time %10.3f[ms] for EXC [%s:%02hu]",
				last_cycle_ms, prec->name.c_str(), prec->exc_id);

	// Update running counters
	pstats->time_processing += last_cycle_ms;
	prec->time_processing += last_cycle_ms;

	// Push sample into accumulator
	pstats->cycle_samples(last_cycle_ms);

	// Statistic features extraction for cycle time estimation:
	DB(
	uint32_t _count = count(pstats->cycle_samples);
	double _min = min(pstats->cycle_samples);
	double _max = max(pstats->cycle_samples);
	double _avg = mean(pstats->cycle_samples);
	double _var = variance(pstats->cycle_samples);
	logger->Debug("Cycle #%08d: m: %.3f, M: %.3f, a: %.3f, v: %.3f) [ms]",
		_count, _min, _max, _avg, _var);
	)

	// TODO: here we can get the overhead for statistica analysis
	// by re-reading the timer and comparing with the preivously readed
	// value

	// TIMER: Re-sart RUNNING
	prec->exc_tmr.start();

}

void BbqueRPC::SyncTimeEstimation(pregExCtx_t prec) {
	pAwmStats_t pstats(prec->pAwmStats);
	// Check if we already ran on this AWM
	if (unlikely(!pstats)) {
		// This condition is verified just when we entered a SYNC
		// before sending a GWM. In this case, statistics have not
		// yet been setup
		return;
	}
	_SyncTimeEstimation(prec);
}

RTLIB_ExitCode_t BbqueRPC::UpdateStatistics(pregExCtx_t prec) {
	pAwmStats_t pstats(prec->pAwmStats);
	double last_config_ms;

	// Check if this is the first re-start on this AWM
	if (!isSyncDone(prec)) {
		// TIMER: Get RECONF
		last_config_ms = prec->exc_tmr.getElapsedTimeMs();
		prec->time_config += last_config_ms;

		// Reconfiguration time statistics collection
		pstats->time_configuring += last_config_ms;
		pstats->config_samples(last_config_ms);

		// Statistic features extraction for cycle time estimation:
		DB(
		uint32_t _count = count(pstats->config_samples);
		double _min = min(pstats->config_samples);
		double _max = max(pstats->config_samples);
		double _avg = mean(pstats->config_samples);
		double _var = variance(pstats->config_samples);
		logger->Debug("Config #%08d: m: %.3f, M: %.3f, a: %.3f, v: %.3f) [ms]",
			_count, _min, _max, _avg, _var);
		)

		// TIMER: Sart RUNNING
		prec->exc_tmr.start();
		return RTLIB_OK;
	}

	// Update sync time estimation
	SyncTimeEstimation(prec);

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::GetAssignedWorkingMode(
		pregExCtx_t prec,
		RTLIB_WorkingModeParams_t *wm) {
	std::unique_lock<std::mutex> rec_ul(prec->mtx);

	if (!isEnabled(prec)) {
		logger->Debug("Get AWM FAILED (Error: EXC not enabled)");
		return RTLIB_EXC_GWM_FAILED;
	}

	if (isBlocked(prec)) {
		logger->Debug("BLOCKED");
		return RTLIB_EXC_GWM_BLOCKED;
	}

	if (isSyncMode(prec) && !isAwmValid(prec)) {
		logger->Debug("SYNC Pending");
		// Update AWM statistics
		// This is required to save the sync_time of the last
		// completed cycle, thus having a correct cycles count
		SyncTimeEstimation(prec);
		return RTLIB_EXC_SYNC_MODE;
	}

	if (!isSyncMode(prec) && !isAwmValid(prec)) {
		// This is the case to send a GWM to Barbeque
		logger->Debug("NOT valid AWM");
		return RTLIB_EXC_GWM_FAILED;
	}

	logger->Debug("Valid AWM assigned");
	wm->awm_id = prec->awm_id;

	// Update AWM statistics
	UpdateStatistics(prec);

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::WaitForWorkingMode(
		pregExCtx_t prec,
		RTLIB_WorkingModeParams_t *wm) {
	std::unique_lock<std::mutex> rec_ul(prec->mtx);

	// Shortcut in case the AWM has been already assigned
	if (isAwmAssigned(prec))
		goto waiting_done;

	// Notify we are going to be suspended waiting for an AWM
	setAwmWaiting(prec);

	// TIMER: Start BLOCKED
	prec->exc_tmr.start();

	// Wait for the EXC being un-BLOCKED
	if (isBlocked(prec))
		while (isBlocked(prec))
			prec->cv.wait(rec_ul);
	else
	// Wait for the EXC being assigned an AWM
		while (isEnabled(prec) && !isAwmAssigned(prec) && !isBlocked(prec))
			prec->cv.wait(rec_ul);

	clearAwmWaiting(prec);

	// TIMER: Get BLOCKED
	prec->time_blocked += prec->exc_tmr.getElapsedTimeMs();

waiting_done:

	// TIMER: Sart RECONF
	prec->exc_tmr.start();

	setAwmValid(prec);
	wm->awm_id = prec->awm_id;

	// Setup AWM statistics
	SetupStatistics(prec);

	return RTLIB_OK;
}


RTLIB_ExitCode_t BbqueRPC::WaitForSyncDone(pregExCtx_t prec) {
	std::unique_lock<std::mutex> rec_ul(prec->mtx);

	while (isEnabled(prec) && !isSyncDone(prec)) {
		logger->Debug("Waiting for reconfiguration to complete...");
		prec->cv.wait(rec_ul);
	}

	// TODO add a timeout wait to limit the maximum reconfiguration time
	// before notifying an anomaly to the RTRM

	clearSyncMode(prec);

	return RTLIB_OK;
}


RTLIB_ExitCode_t BbqueRPC::GetWorkingMode(
		const RTLIB_ExecutionContextHandler_t ech,
		RTLIB_WorkingModeParams_t *wm,
		RTLIB_SyncType_t st) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;
	// FIXME Remove compilation warning
	(void)st;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Getting WM for EXC [%p] FAILED "
			"(Error: EXC not registered)", (void*)ech);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	if (unlikely(prec->ctrlTrdPid == 0)) {
		// Keep track of the Control Thread PID
		prec->ctrlTrdPid = gettid();
		logger->Debug("Tracking control thread PID [%d] for EXC [%d]...",
					prec->ctrlTrdPid, prec->exc_id);
	}

#ifdef CONFIG_BBQUE_RTLIB_UNMANAGED_SUPPORT

	if (!envUnmanaged)
		goto do_gwm;

	// Configuration already done
	if (isAwmValid(prec)) {
		setSyncDone(prec);
		return RTLIB_OK;
	}

	// Configure unmanaged EXC in AWM0
	prec->event = RTLIB_EXC_GWM_START;
	wm->awm_id = envUnmanagedAWM;
	setAwmValid(prec);
	goto do_reconf;

do_gwm:

#endif

	// Checking if a valid AWM has been assigned
	logger->Debug("Looking for assigned AWM...");
	result = GetAssignedWorkingMode(prec, wm);
	if (result == RTLIB_OK) {
		setSyncDone(prec);
		// Notify about synchronization completed
		prec->cv.notify_one();
		return RTLIB_OK;
	}

	// Checking if the EXC has been blocked
	if (result == RTLIB_EXC_GWM_BLOCKED) {
		setSyncDone(prec);
		// Notify about synchronization completed
		prec->cv.notify_one();
	}

	// Exit if the EXC has been disabled
	if (!isEnabled(prec))
		return RTLIB_EXC_GWM_FAILED;

	if (!isSyncMode(prec) && (result == RTLIB_EXC_GWM_FAILED)) {

		logger->Debug("AWM not assigned, sending schedule request to RTRM...");
		DB(logger->Info("[%s:%02hu] ===> BBQ::ScheduleRequest()",
				prec->name.c_str(), prec->exc_id));
		// Calling the low-level start
		result = _ScheduleRequest(prec);
		if (result != RTLIB_OK)
			goto exit_gwm_failed;
	} else {
		// At this point, the EXC should be either in Synchronization Mode
		// or Blocked, and thus it should wait for an EXC being
		// assigned by the RTRM
		assert((result == RTLIB_EXC_SYNC_MODE) ||
				(result == RTLIB_EXC_GWM_BLOCKED));
	}

	logger->Debug("Waiting for assigned AWM...");

	// Waiting for an AWM being assigned
	result = WaitForWorkingMode(prec, wm);
	if (result != RTLIB_OK)
		goto exit_gwm_failed;

	// Compilation warning fix
	goto do_reconf;
do_reconf:

	// Exit if the EXC has been disabled
	if (!isEnabled(prec))
		return RTLIB_EXC_GWM_FAILED;

	// Processing the required reconfiguration action
	switch(prec->event) {
	case RTLIB_EXC_GWM_START:
		// Keep track of the CGroup path
		// CGroups are created at the first allocation of resources to this
		// application, thus we could check for them right after the
		// first AWM has been assinged.
		SetCGroupPath(prec);
		// Here, the missing "break" is not an error ;-)
	case RTLIB_EXC_GWM_RECONF:
	case RTLIB_EXC_GWM_MIGREC:
	case RTLIB_EXC_GWM_MIGRATE:
		DB(logger->Info("[%s:%02hu] <------------- AWM [%02d] --",
				prec->name.c_str(), prec->exc_id, prec->awm_id));
		break;
	case RTLIB_EXC_GWM_BLOCKED:
		DB(logger->Info("[%s:%02hu] <---------------- BLOCKED --",
				prec->name.c_str(), prec->exc_id));
		break;
	default:
		DB(logger->Error("Execution context [%s] GWM FAILED "
					"(Error: Invalid event [%d])",
					prec->name.c_str(), prec->event));
		assert(prec->event >= RTLIB_EXC_GWM_START);
		assert(prec->event <= RTLIB_EXC_GWM_BLOCKED);
		break;
	}

	return prec->event;

exit_gwm_failed:

	DB(logger->Error("Execution context [%s] GWM FAILED (Error %d: %s)",
				prec->name.c_str(), result, RTLIB_ErrorStr(result)));
	return RTLIB_EXC_GWM_FAILED;

}

uint32_t BbqueRPC::GetSyncLatency(pregExCtx_t prec) {
	pAwmStats_t pstats = prec->pAwmStats;
	double elapsedTime;
	double syncDelay;
	double _avg;
	double _var;

	// Get the statistics for the current AWM
	assert(pstats);
	std::unique_lock<std::mutex> stats_ul(pstats->stats_mtx);
	_avg = mean(pstats->cycle_samples);
	_var = variance(pstats->cycle_samples);
	stats_ul.unlock();

	// Compute a reasonale sync point esimation
	// we assume a NORMAL distribution of execution times
	syncDelay = _avg + (10 * sqrt(_var));

	// Discount the already passed time since lasy sync point
	elapsedTime = prec->exc_tmr.getElapsedTimeMs();
	if (elapsedTime < syncDelay)
		syncDelay -= elapsedTime;
	else
		syncDelay = 0;

	logger->Debug("Expected sync time in %10.3f[ms] for EXC [%s:%02hu]",
				syncDelay, prec->name.c_str(), prec->exc_id);

	return ceil(syncDelay);
}


/******************************************************************************
 * Synchronization Protocol Messages
 ******************************************************************************/

RTLIB_ExitCode_t BbqueRPC::SyncP_PreChangeNotify(pregExCtx_t prec) {
	// Entering Synchronization mode
	setSyncMode(prec);
	// Resetting Sync Done
	clearSyncDone(prec);
	// Setting current AWM as invalid
	clearAwmValid(prec);
	clearAwmAssigned(prec);
	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::SyncP_PreChangeNotify(
		rpc_msg_BBQ_SYNCP_PRECHANGE_t &msg) {
	RTLIB_ExitCode_t result;
	uint32_t syncLatency;
	pregExCtx_t prec;

	prec = getRegistered(msg.hdr.exc_id);
	if (!prec) {
		logger->Error("SyncP_1 (Pre-Change) EXC [%d] FAILED "
				"(Error: Execution Context not registered)",
				msg.hdr.exc_id);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	assert(msg.event < ba::ApplicationStatusIF::SYNC_STATE_COUNT);

	std::unique_lock<std::mutex> rec_ul(prec->mtx);

	// Keep copy of the required synchronization action
	prec->event = (RTLIB_ExitCode_t)(RTLIB_EXC_GWM_START + msg.event);

	result = SyncP_PreChangeNotify(prec);

	// Set the new required AWM (if not being blocked)
	if (prec->event != RTLIB_EXC_GWM_BLOCKED) {
		prec->awm_id = msg.awm;
#ifdef CONFIG_BBQUE_OPENCL
		prec->dev_id = msg.dev;
#endif
		logger->Info("SyncP_1 (Pre-Change) EXC [%d], Action [%d], Assigned AWM [%d]",
				msg.hdr.exc_id, msg.event, msg.awm);
	} else {
		logger->Info("SyncP_1 (Pre-Change) EXC [%d], Action [%d:BLOCKED]",
				msg.hdr.exc_id, msg.event);
	}

	// FIXME add a string representation of the required action

	syncLatency = 0;
	if (!isAwmWaiting(prec) && prec->pAwmStats) {
		// Update the Synchronziation Latency
		syncLatency = GetSyncLatency(prec);
	}

	rec_ul.unlock();

	logger->Debug("SyncP_1 (Pre-Change) EXC [%d], SyncLatency [%u]",
				msg.hdr.exc_id, syncLatency);

	result = _SyncpPreChangeResp(msg.hdr.token, prec, syncLatency);


#ifndef CONFIG_BBQUE_YM_SYNC_FORCE

	if (result != RTLIB_OK)
		return result;

	// Force a DoChange, which will not be forwarded by the BBQ daemon if
	// the Sync Point forcing support is disabled
	logger->Info("SyncP_3 (Do-Change) EXC [%d]", msg.hdr.exc_id);
	return SyncP_DoChangeNotify(prec);

#else

	return result;

#endif
}

RTLIB_ExitCode_t BbqueRPC::SyncP_SyncChangeNotify(pregExCtx_t prec) {
	std::unique_lock<std::mutex> rec_ul(prec->mtx);
	// Checking if the apps is in Sync Status
	if (!isAwmWaiting(prec))
		return RTLIB_EXC_SYNCP_FAILED;

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::SyncP_SyncChangeNotify(
		rpc_msg_BBQ_SYNCP_SYNCCHANGE_t &msg) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;

	prec = getRegistered(msg.hdr.exc_id);
	if (!prec) {
		logger->Error("SyncP_2 (Sync-Change) EXC [%d] FAILED "
				"(Error: Execution Context not registered)",
				msg.hdr.exc_id);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	result = SyncP_SyncChangeNotify(prec);
	if (result != RTLIB_OK) {
		logger->Warn("SyncP_2 (Sync-Change) EXC [%d] CRITICAL "
				"(Warning: Overpassing Synchronization time)",
				msg.hdr.exc_id);
	}

	logger->Info("SyncP_2 (Sync-Change) EXC [%d]",
			msg.hdr.exc_id);

	_SyncpSyncChangeResp(msg.hdr.token, prec, result);

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::SyncP_DoChangeNotify(pregExCtx_t prec) {
	std::unique_lock<std::mutex> rec_ul(prec->mtx);

	// Update the EXC status based on the last required re-configuration action
	if (prec->event == RTLIB_EXC_GWM_BLOCKED) {
		setBlocked(prec);
	} else {
		clearBlocked(prec);
		setAwmAssigned(prec);
	}

	// TODO Setup the ground for reconfiguration statistics collection
	// TODO Start the re-configuration by notifying the waiting thread
	prec->cv.notify_one();

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::SyncP_DoChangeNotify(
		rpc_msg_BBQ_SYNCP_DOCHANGE_t &msg) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;

	prec = getRegistered(msg.hdr.exc_id);
	if (!prec) {
		logger->Error("SyncP_3 (Do-Change) EXC [%d] FAILED "
				"(Error: Execution Context not registered)",
				msg.hdr.exc_id);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	result = SyncP_DoChangeNotify(prec);

	// NOTE this command should not generate a response, it is just a notification

	logger->Info("SyncP_3 (Do-Change) EXC [%d]", msg.hdr.exc_id);

	return result;
}

RTLIB_ExitCode_t BbqueRPC::SyncP_PostChangeNotify(pregExCtx_t prec) {
	// TODO Wait for the apps to end its reconfiguration
	// TODO Collect stats on reconfiguraiton time
	return WaitForSyncDone(prec);
}

RTLIB_ExitCode_t BbqueRPC::SyncP_PostChangeNotify(
		rpc_msg_BBQ_SYNCP_POSTCHANGE_t &msg) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;

	prec = getRegistered(msg.hdr.exc_id);
	if (!prec) {
		logger->Error("SyncP_4 (Post-Change) EXC [%d] FAILED "
				"(Error: Execution Context not registered)",
				msg.hdr.exc_id);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	result = SyncP_PostChangeNotify(prec);
	if (result != RTLIB_OK) {
		logger->Warn("SyncP_4 (Post-Change) EXC [%d] CRITICAL "
				"(Warning: Reconfiguration timeout)",
				msg.hdr.exc_id);
	}

	logger->Info("SyncP_4 (Post-Change) EXC [%d]", msg.hdr.exc_id);

	_SyncpPostChangeResp(msg.hdr.token, prec, result);

	return RTLIB_OK;
}

/******************************************************************************
 * Channel Independant interface
 ******************************************************************************/

RTLIB_ExitCode_t BbqueRPC::Set(
		const RTLIB_ExecutionContextHandler_t ech,
		RTLIB_Constraint_t* constraints,
		uint8_t count) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Constraining EXC [%p] "
				"(Error: EXC not registered)", (void*)ech);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	// Calling the low-level enable function
	result = _Set(prec, constraints, count);
	if (result != RTLIB_OK) {
		DB(logger->Error("Constraining EXC [%p:%s] FAILED (Error %d: %s)",
				(void*)ech, prec->name.c_str(), result, RTLIB_ErrorStr(result)));
		return RTLIB_EXC_ENABLE_FAILED;
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::Clear(
		const RTLIB_ExecutionContextHandler_t ech) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Clear constraints for EXC [%p] "
				"(Error: EXC not registered)", (void*)ech);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	// Calling the low-level enable function
	result = _Clear(prec);
	if (result != RTLIB_OK) {
		DB(logger->Error("Clear constraints for EXC [%p:%s] FAILED (Error %d: %s)",
				(void*)ech, prec->name.c_str(), result, RTLIB_ErrorStr(result)));
		return RTLIB_EXC_ENABLE_FAILED;
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::GGap(
		const RTLIB_ExecutionContextHandler_t ech,
		uint8_t percent) {
	RTLIB_ExitCode_t result;
	pregExCtx_t prec;

	assert(ech);

	// Enforce the Goal-Gap domain
	if (unlikely(percent > 100)) {
		logger->Error("Set Goal-Gap for EXC [%p] "
				"(Error: out-of-bound)", (void*)ech);
		return RTLIB_ERROR;
	}

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Set Goal-Gap for EXC [%p] "
				"(Error: EXC not registered)", (void*)ech);
		return RTLIB_EXC_NOT_REGISTERED;
	}

	// Calling the low-level enable function
	result = _GGap(prec, percent);
	if (result != RTLIB_OK) {
		logger->Error("Set Goal-Gap for EXC [%p:%s] FAILED (Error %d: %s)",
				(void*)ech, prec->name.c_str(), result,RTLIB_ErrorStr(result));
		return RTLIB_EXC_ENABLE_FAILED;
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t BbqueRPC::StopExecution(
		RTLIB_ExecutionContextHandler_t ech,
		struct timespec timeout) {
	//Silence "args not used" warning.
	(void)ech;
	(void)timeout;

	return RTLIB_OK;
}

/*******************************************************************************
 *    Performance Monitoring Support
 ******************************************************************************/
#ifdef CONFIG_BBQUE_RTLIB_PERF_SUPPORT

BbqueRPC::PerfEventAttr_t * BbqueRPC::raw_events = nullptr;

BbqueRPC::PerfEventAttr_t BbqueRPC::default_events[] = {

  {PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK },
  {PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES },
  {PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS },
  {PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS },

  {PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES	},
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)
  {PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND },
  {PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND },
#endif
  {PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS },
  {PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS },
  {PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES },

};


BbqueRPC::PerfEventAttr_t BbqueRPC::detailed_events[] = {

  {PERF_TYPE_HW_CACHE, L1DC_RA },
  {PERF_TYPE_HW_CACHE, L1DC_RM },
  {PERF_TYPE_HW_CACHE, LLC_RA },
  {PERF_TYPE_HW_CACHE, LLC_RM },

};


BbqueRPC::PerfEventAttr_t BbqueRPC::very_detailed_events[] = {

  {PERF_TYPE_HW_CACHE,	L1IC_RA },
  {PERF_TYPE_HW_CACHE,	L1IC_RM },
  {PERF_TYPE_HW_CACHE,	DTLB_RA },
  {PERF_TYPE_HW_CACHE,	DTLB_RM },
  {PERF_TYPE_HW_CACHE,	ITLB_RA },
  {PERF_TYPE_HW_CACHE,	ITLB_RM },

};

BbqueRPC::PerfEventAttr_t BbqueRPC::very_very_detailed_events[] = {

  {PERF_TYPE_HW_CACHE,	L1DC_PA },
  {PERF_TYPE_HW_CACHE,	L1DC_PM },

};

static const char *_perfCounterName = 0;

uint8_t BbqueRPC::InsertRAWPerfCounter(const char *perf_str) {
	static uint8_t idx = 0;
	uint64_t event_code_ul;
	char event_code_str[4];
	char label[10];
	char buff[15];

	// Overflow check
	if (idx == envRawPerfCount)
		return idx;

	// Extract label and event select code + unit mask
	strncpy(buff, perf_str, sizeof(buff));
	strncpy(strpbrk(buff,"-"), " ", 1);
	sscanf(buff, "%10s %4s", label, event_code_str);

	// Nullptr check
	if ((label == nullptr) || (event_code_str == nullptr))
		return 0;

	// Convert the event code from string to unsigned long
	event_code_ul = strtoul(event_code_str, NULL, 16);

	// Allocate the raw events array
	if (!raw_events)
		raw_events = (PerfEventAttr_t *)
			malloc(sizeof(PerfEventAttr_t) * envRawPerfCount);

	// Set the event attributes
	raw_events[idx++] = {PERF_TYPE_RAW, event_code_ul};

	return idx;
}

BbqueRPC::pPerfEventStats_t BbqueRPC::PerfGetEventStats(pAwmStats_t pstats,
		perf_type_id type, uint64_t config) {
	PerfEventStatsMapByConf_t::iterator it;
	pPerfEventStats_t ppes;
	uint8_t conf_key, curr_key;

	conf_key = (uint8_t)(0xFF & config);

	// LookUp for requested event
	it = pstats->events_conf_map.lower_bound(conf_key);
	for ( ; it != pstats->events_conf_map.end(); ++it) {
		ppes = (*it).second;

		// Check if current (conf) key is still valid
		curr_key = (uint8_t)(0xFF & ppes->pattr->config);
		if (curr_key != conf_key)
			break;

		// Check if we found the required event
		if ((ppes->pattr->type == type) &&
			(ppes->pattr->config == config))
			return ppes;
	}

	return pPerfEventStats_t();
}

void BbqueRPC::PerfSetupEvents(pregExCtx_t prec) {
	uint8_t tot_counters = ARRAY_SIZE(default_events);
	int fd;

	// TODO get required Perf configuration from environment
	// to add eventually more detailed counters or to completely disable perf
	// support

	// Adding raw events
	for (uint8_t e = 0; e < envRawPerfCount; e++) {
		fd = prec->perf.AddCounter(
				PERF_TYPE_RAW, raw_events[e].config, envNoKernel);
		prec->events_map[fd] = &(raw_events[e]);
    }

	// RAW events mode skip the preset counters
	if (envRawPerfCount > 0)
		return;

	// Adding default events
	for (uint8_t e = 0; e < tot_counters; e++) {
		fd = prec->perf.AddCounter(
				default_events[e].type, default_events[e].config,
				envNoKernel);
		prec->events_map[fd] = &(default_events[e]);
	}

	if (envDetailedRun <  1)
		return;

	// Append detailed run extra attributes
	for (uint8_t e = 0; e < ARRAY_SIZE(detailed_events); e++) {
		fd = prec->perf.AddCounter(
				detailed_events[e].type, detailed_events[e].config,
				envNoKernel);
		prec->events_map[fd] = &(detailed_events[e]);
	}

	if (envDetailedRun <  2)
		return;

	// Append detailed run extra attributes
	for (uint8_t e = 0; e < ARRAY_SIZE(very_detailed_events); e++) {
		fd = prec->perf.AddCounter(
				very_detailed_events[e].type, very_detailed_events[e].config,
				envNoKernel);
		prec->events_map[fd] = &(very_detailed_events[e]);
	}

	if (envDetailedRun <  3)
		return;

	// Append detailed run extra attributes
	for (uint8_t e = 0; e < ARRAY_SIZE(very_very_detailed_events); e++) {
		fd = prec->perf.AddCounter(
				very_very_detailed_events[e].type, very_very_detailed_events[e].config,
				envNoKernel);
		prec->events_map[fd] = &(very_very_detailed_events[e]);
	}
}

void BbqueRPC::PerfSetupStats(pregExCtx_t prec, pAwmStats_t pstats) {
	PerfRegisteredEventsMap_t::iterator it;
	pPerfEventStats_t ppes;
	pPerfEventAttr_t ppea;
	uint8_t conf_key;
	int fd;

	if (PerfRegisteredEvents(prec) == 0)
		return;

	// Setup performance counters
	for (it = prec->events_map.begin(); it != prec->events_map.end(); ++it) {
		fd = (*it).first;
		ppea = (*it).second;

		// Build new perf counter statistics
		ppes = pPerfEventStats_t(new PerfEventStats_t());
		assert(ppes);
		ppes->id = fd;
		ppes->pattr = ppea;

		// Keep track of perf statistics for this AWM
		pstats->events_map[fd] = ppes;

		// Index statistics by configuration (radix)
		conf_key = (uint8_t)(0xFF & ppea->config);
		pstats->events_conf_map.insert(
			PerfEventStatsMapByConfEntry_t(conf_key, ppes));
	}

}

void BbqueRPC::PerfCollectStats(pregExCtx_t prec) {
	std::unique_lock<std::mutex> stats_ul(prec->pAwmStats->stats_mtx);
	pAwmStats_t pstats = prec->pAwmStats;
	PerfEventStatsMap_t::iterator it;
	pPerfEventStats_t ppes;
	uint64_t delta;
	int fd;

	// Collect counters for registered events
	it = pstats->events_map.begin();
	for ( ; it != pstats->events_map.end(); ++it) {
		ppes = (*it).second;
		fd = (*it).first;

		// Reading delta for this perf counter
		delta = prec->perf.Update(fd);

		// Computing stats for this counter
		ppes->value += delta;
		ppes->perf_samples(delta);
	}

}

void BbqueRPC::PerfPrintNsec(pAwmStats_t pstats, pPerfEventStats_t ppes) {
	pPerfEventAttr_t ppea = ppes->pattr;
	double avg = mean(ppes->perf_samples);
	double total, ratio = 0.0;
	double msecs = avg / 1e6;

	if (envMOSTOutput)
		DUMP_MOST_METRIC("perf", _perfCounterName, msecs, "%.6f");
	else
		fprintf(outfd, "%19.6f%s%-25s", msecs, envCsvSep,
			bu::Perf::EventName(ppea->type, ppea->config));

	if (envCsvOutput)
		return;

	if (PerfEventMatch(ppea, PERF_SW(TASK_CLOCK))) {
		// Get AWM average running time
		total = mean(pstats->cycle_samples) * 1e6;

		if (total) {
			ratio = avg / total;

			if (envMOSTOutput)
				DUMP_MOST_METRIC("perf", "cpu_utiliz", ratio, "%.3f");
			else
				fprintf(outfd, " # %8.3f CPUs utilized          ", ratio);
			return;
		}
	}

}

void BbqueRPC::PerfPrintMissesRatio(double avg_missed, double tot_branches, const char *text) {
	double ratio = 0.0;
	const char *color;

	if (tot_branches)
		ratio = avg_missed / tot_branches * 100.0;

	color = PERF_COLOR_NORMAL;
	if (ratio > 20.0)
		color = PERF_COLOR_RED;
	else if (ratio > 10.0)
		color = PERF_COLOR_MAGENTA;
	else if (ratio > 5.0)
		color = PERF_COLOR_YELLOW;

	fprintf(outfd, " #  ");
	bu::Perf::FPrintf(outfd, color, "%6.2f%%", ratio);
	fprintf(outfd, " %-23s", text);

}

void BbqueRPC::PerfPrintAbs(pAwmStats_t pstats, pPerfEventStats_t ppes) {
	pPerfEventAttr_t ppea = ppes->pattr;
	double avg = mean(ppes->perf_samples);
	double total, total2, ratio = 0.0;
	pPerfEventStats_t ppes2;
	const char *fmt;

	// shutdown compiler warnings for kernels < 3.1
	(void)total2;

	if (envMOSTOutput) {
		DUMP_MOST_METRIC("perf", _perfCounterName, avg, "%.0f");
	} else {
		if (envCsvOutput)
			fmt = "%.0f%s%s";
		else if (envBigNum)
			fmt = "%'19.0f%s%-25s";
		else
			fmt = "%19.0f%s%-25s";

		fprintf(outfd, fmt, avg, envCsvSep,
				bu::Perf::EventName(ppea->type, ppea->config));
	}

	if (envCsvOutput)
		return;

	if (PerfEventMatch(ppea, PERF_HW(INSTRUCTIONS))) {

		// Compute "instructions per cycle"
		ppes2 = PerfGetEventStats(pstats, PERF_HW(CPU_CYCLES));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);

		if (total)
			ratio = avg / total;

		if (envMOSTOutput) {
			DUMP_MOST_METRIC("perf", "ipc", ratio, "%.2f");
		} else {
			fprintf(outfd, " #   %5.2f  insns per cycle        ", ratio);
		}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)
		// Compute "stalled cycles per instruction"
		ppes2 = PerfGetEventStats(pstats, PERF_HW(STALLED_CYCLES_FRONTEND));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);

		ppes2 = PerfGetEventStats(pstats, PERF_HW(STALLED_CYCLES_BACKEND));
		if (!ppes2)
			return;
		total2 = mean(ppes2->perf_samples);
		if (total < total2)
			total = total2;

		if (total && avg) {
			ratio = total / avg;
			if (envMOSTOutput) {
				DUMP_MOST_METRIC("perf", "stall_cycles_per_inst", avg, "%.0f");
			} else {
				fprintf(outfd, "\n%45s#   %5.2f  stalled cycles per insn", " ", ratio);
			}
		}
#endif
		return;
	}

	if (!envMOSTOutput && PerfEventMatch(ppea, PERF_HW(BRANCH_MISSES))) {
		ppes2 = PerfGetEventStats(pstats, PERF_HW(BRANCH_INSTRUCTIONS));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);
		if (total)
			PerfPrintMissesRatio(avg, total, "of all branches");

		return;
	}

	if (!envMOSTOutput && PerfEventMatch(ppea, PERF_HC(L1DC_RM))) {
		ppes2 = PerfGetEventStats(pstats, PERF_HC(L1DC_RA));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);
		if (total)
			PerfPrintMissesRatio(avg, total, "of all L1-dcache hits");

		return;
	}

	if (!envMOSTOutput && PerfEventMatch(ppea, PERF_HC(L1IC_RM))) {
		ppes2 = PerfGetEventStats(pstats, PERF_HC(L1IC_RA));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);
		if (total)
			PerfPrintMissesRatio(avg, total, "of all L1-icache hits");

		return;
	}

	if (!envMOSTOutput && PerfEventMatch(ppea, PERF_HC(DTLB_RM))) {
		ppes2 = PerfGetEventStats(pstats, PERF_HC(DTLB_RA));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);
		if (total)
			PerfPrintMissesRatio(avg, total, "of all dTLB cache hits");

		return;
	}

	if (!envMOSTOutput && PerfEventMatch(ppea, PERF_HC(ITLB_RM))) {
		ppes2 = PerfGetEventStats(pstats, PERF_HC(ITLB_RA));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);
		if (total)
			PerfPrintMissesRatio(avg, total, "of all iTLB cache hits");

		return;
	}

	if (!envMOSTOutput && PerfEventMatch(ppea, PERF_HC(LLC_RM))) {
		ppes2 = PerfGetEventStats(pstats, PERF_HC(LLC_RA));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);
		if (total)
			PerfPrintMissesRatio(avg, total, "of all LL-cache hits");

		return;
	}

	if (!envMOSTOutput && PerfEventMatch(ppea, PERF_HW(CACHE_MISSES))) {
		ppes2 = PerfGetEventStats(pstats, PERF_HW(CACHE_REFERENCES));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);
		if (total) {
			ratio = avg * 100 / total;
		    fprintf(outfd, " # %8.3f %% of all cache refs    ", ratio);
		}

		return;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)
	// TODO add CPU front/back-end stall stats
#endif

	if (PerfEventMatch(ppea, PERF_HW(CPU_CYCLES))) {
		ppes2 = PerfGetEventStats(pstats, PERF_SW(TASK_CLOCK));
		if (!ppes2)
			return;
		total = mean(ppes2->perf_samples);
		if (total) {
			ratio = 1.0 * avg / total;
			if (envMOSTOutput) {
				DUMP_MOST_METRIC("perf", "ghz", ratio, "%.3f");
			} else {
				fprintf(outfd, " # %8.3f GHz                    ", ratio);
			}
		}

		return;
	}

	// In MOST output mode, here we return
	if (envMOSTOutput)
		return;

	// By default print the frequency of the event in [M/sec]
	ppes2 = PerfGetEventStats(pstats, PERF_SW(TASK_CLOCK));
	if (!ppes2)
		return;
	total = mean(ppes2->perf_samples);
	if (total) {
		ratio = 1000.0 * avg / total;
		fprintf(outfd, " # %8.3f M/sec                  ", ratio);
		return;
	}

	// Otherwise, simply generate an empty line
	fprintf(outfd, "%-35s", " ");

}

bool BbqueRPC::IsNsecCounter(pregExCtx_t prec, int fd) {
	pPerfEventAttr_t ppea = prec->events_map[fd];
	if (PerfEventMatch(ppea, PERF_SW(CPU_CLOCK)) ||
		PerfEventMatch(ppea, PERF_SW(TASK_CLOCK)))
		return true;
	return false;
}

void BbqueRPC::PerfPrintStats(pregExCtx_t prec, pAwmStats_t pstats) {
	PerfEventStatsMap_t::iterator it;
	pPerfEventStats_t ppes;
	pPerfEventAttr_t ppea;
	uint64_t avg_enabled, avg_running;
	double avg_value, std_value;
	int fd;

	// For each registered counter
	for (it = pstats->events_map.begin(); it != pstats->events_map.end(); ++it) {
		ppes = (*it).second;
		fd = (*it).first;

		// Keep track of current Performance Counter name
		ppea = prec->events_map[fd];
		_perfCounterName = bu::Perf::EventName(ppea->type, ppea->config);

		if (IsNsecCounter(prec, fd))
			PerfPrintNsec(pstats, ppes);
		else
			PerfPrintAbs(pstats, ppes);

		// Print stddev ratio
		if (count(ppes->perf_samples) > 1) {

			// Get AWM average and stddev running time
			avg_value = mean(ppes->perf_samples);
			std_value = sqrt(static_cast<double>(variance(ppes->perf_samples)));

			PrintNoisePct(std_value, avg_value);
		}

		// Computing counter scheduling statistics
		avg_enabled = prec->perf.Enabled(ppes->id, false);
		avg_running = prec->perf.Running(ppes->id, false);

		// In MOST output mode, always dump counter usage percentage
		if (envMOSTOutput) {
			char buff[64];
			snprintf(buff, 64, "%s_pcu", _perfCounterName);
			DUMP_MOST_METRIC("perf", buff,
					(100.0 * avg_running / avg_enabled),
					"%.2f");
			continue;
		}

		DB(fprintf(outfd, " (Ena: %20lu, Run: %10lu) ", avg_enabled, avg_running));

		// Print percentage of counter usage
		if (avg_enabled != avg_running) {
			fprintf(outfd, " [%5.2f%%]", 100.0 * avg_running / avg_enabled);
		}

		fputc('\n', outfd);
	}

	// In MOST output mode, no more metrics are dumped
	if (envMOSTOutput)
		return;

	if (!envCsvOutput) {

		fputc('\n', outfd);

		// Get AWM average and stddev running time
		avg_value = mean(pstats->cycle_samples);
		std_value = sqrt(static_cast<double>(variance(pstats->cycle_samples)));

		fprintf(outfd, " %18.6f cycle time [ms]", avg_value);
		if (count(pstats->cycle_samples) > 1) {
			fprintf(outfd, "                                        ");
			PrintNoisePct(std_value, avg_value);
		}
		fprintf(outfd, "\n\n");
	}

}

void BbqueRPC::PrintNoisePct(double total, double avg) {
	const char *color;
	double pct = 0.0;

	if (avg)
		pct = 100.0*total/avg;

	if (envMOSTOutput) {
		char buff[64];
		snprintf(buff, 64, "%s_pct", _perfCounterName);
		DUMP_MOST_METRIC("perf", buff, pct, "%.2f");
		return;
	}


	if (envCsvOutput) {
		fprintf(outfd, "%s%.2f%%", envCsvSep, pct);
		return;
	}

	color = PERF_COLOR_NORMAL;
	if (pct > 80.0)
		color = PERF_COLOR_RED;
	else if (pct > 60.0)
		color = PERF_COLOR_MAGENTA;
	else if (pct > 40.0)
		color = PERF_COLOR_YELLOW;

	fprintf(outfd, "  ( ");
	bu::Perf::FPrintf(outfd, color, "+-%6.2f%%", pct);
	fprintf(outfd, " )");
}

#endif // CONFIG_BBQUE_RTLIB_PERF_SUPPORT

/*******************************************************************************
 *    OpenCL support
 ******************************************************************************/
#ifdef CONFIG_BBQUE_OPENCL

#define SUM(v) \
	sum(it_ct->second[CL_CMD_ ## v ## _TIME])*1e-06
#define MIN(v) \
	min(it_ct->second[CL_CMD_ ## v ## _TIME])*1e-06
#define MAX(v) \
	max(it_ct->second[CL_CMD_ ## v ## _TIME])*1e-06
#define MEAN(v) \
	mean(it_ct->second[CL_CMD_ ## v ## _TIME])*1e-06
#define STDDEV(v) \
	sqrt(static_cast<double>(variance(it_ct->second[CL_CMD_ ## v ## _TIME])))*1e-06

void BbqueRPC::OclSetDevice(uint8_t device_id, RTLIB_ExitCode_t status) {
	rtlib_ocl_set_device(device_id, status);
}

void BbqueRPC::OclClearStats() {
	rtlib_ocl_prof_clean();
}

void BbqueRPC::OclCollectStats(
		uint8_t awm_id,
		OclEventsStatsMap_t & ocl_events_map) {
	rtlib_ocl_prof_run(awm_id, ocl_events_map, envOCLProfLevel);
}

void BbqueRPC::OclPrintStats(pAwmStats_t pstats) {
	std::map<cl_command_queue, QueueProfPtr_t>::iterator it_cq;
	for (it_cq = pstats->ocl_events_map.begin(); it_cq != pstats->ocl_events_map.end(); it_cq++) {
		QueueProfPtr_t stPtr = it_cq->second;
		OclPrintCmdStats(stPtr, it_cq->first);
		if (envOCLProfLevel > 0) {
			fprintf(stderr, FD("OCL: Printing command instance statistics...\n"));
			OclPrintAddrStats(stPtr, it_cq->first);
		}
	}
}

void BbqueRPC::OclPrintCmdStats(QueueProfPtr_t stPtr, cl_command_queue cmd_queue) {
	std::map<cl_command_type, AccArray_t>::iterator it_ct;
	char q_buff[100], s_buff[100], x_buff[100];
	std::string q_str, s_str, x_str;
	for (it_ct = stPtr->cmd_prof.begin(); it_ct != stPtr->cmd_prof.end(); it_ct++) {
		snprintf(q_buff, 100, "%p_%s_queue",
			(void *) cmd_queue, ocl_cmd_str[it_ct->first].c_str());
		q_str.assign(q_buff);
		snprintf(s_buff, 100, "%p_%s_submit",
			(void *) cmd_queue, ocl_cmd_str[it_ct->first].c_str());
		s_str.assign(s_buff);
		snprintf(x_buff, 100, "%p_%s_exec",
			(void *) cmd_queue, ocl_cmd_str[it_ct->first].c_str());
		x_str.assign(x_buff);
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_sum_ms").c_str(), SUM(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_min_ms").c_str(), MIN(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_max_ms").c_str(), MAX(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_avg_ms").c_str(), MEAN(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_std_ms").c_str(), STDDEV(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_sum_ms").c_str(), SUM(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_min_ms").c_str(), MIN(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_max_ms").c_str(), MAX(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_avg_ms").c_str(), MEAN(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_std_ms").c_str(), STDDEV(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_sum_ms").c_str(), SUM(EXEC), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_min_ms").c_str(), MIN(EXEC), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_max_ms").c_str(), MAX(EXEC), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_avg_ms").c_str(), MEAN(EXEC), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_std_ms").c_str(), STDDEV(EXEC), "%.3f");
	}
}

void BbqueRPC::OclPrintAddrStats(QueueProfPtr_t stPtr, cl_command_queue cmd_queue) {
	std::map<void *, AccArray_t>::iterator it_ct;
	char q_buff[100], s_buff[100], x_buff[100];
	cl_command_type cmd_type;
	std::string q_str, s_str, x_str;
	for (it_ct = stPtr->addr_prof.begin(); it_ct != stPtr->addr_prof.end(); it_ct++) {
		cmd_type = rtlib_ocl_get_command_type(it_ct->first);
		snprintf(q_buff, 100, "%p_%s_%p_queue",
			(void *) cmd_queue, ocl_cmd_str[cmd_type].c_str(), it_ct->first);
		q_str.assign(q_buff);
		snprintf(s_buff, 100, "%p_%s_%p_submit",
			(void *) cmd_queue, ocl_cmd_str[cmd_type].c_str(), it_ct->first);
		s_str.assign(s_buff);
		snprintf(x_buff, 100, "%p_%s_%p_exec",
			(void *) cmd_queue, ocl_cmd_str[cmd_type].c_str(), it_ct->first);
		x_str.assign(x_buff);
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_sum_ms").c_str(), SUM(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_min_ms").c_str(), MIN(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_max_ms").c_str(), MAX(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_avg_ms").c_str(), MEAN(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (q_str + "_std_ms").c_str(), STDDEV(QUEUED), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_sum_ms").c_str(), SUM(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_min_ms").c_str(), MIN(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_max_ms").c_str(), MAX(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_avg_ms").c_str(), MEAN(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (s_str + "_std_ms").c_str(), STDDEV(SUBMIT), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_sum_ms").c_str(), SUM(EXEC), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_min_ms").c_str(), MIN(EXEC), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_max_ms").c_str(), MAX(EXEC), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_avg_ms").c_str(), MEAN(EXEC), "%.3f");
		DUMP_MOST_METRIC(CL_TAG, (x_str + "_std_ms").c_str(), STDDEV(EXEC), "%.3f");
	}
}

#define OCL_STATS_HEADER \
"#           Command Queue          ||      Command Type       ||                 queue[ms]                   ||                 submit[ms]                  ||                     exec[ms]                ||\n"\
"# ---------------------------------++-------------------------++---------------------------------------------++---------------------------------------------++---------------------------------------------||\n"\
"#                                  ||                         ||        Ʃ (%%h %%v)      |     μ    |     σ    ||        Ʃ (%%h %%v)      |     μ    |     σ    ||        Ʃ (%%h %%v)      |     μ    |     σ    ||\n"\
"# ---------------------------------++-------------------------++-----------------------+----------+----------++-----------------------+----------+----------++-----------------------+----------+----------||\n"

#define OCL_STATS_HEADER_ADDR \
"#   Command Queue  || Code Address ||      Command Type       ||             queue[ms]          ||            submit[ms]          ||             exec[ms]           ||\n"\
"# -----------------++--------------++-------------------------++--------------------------------++--------------------------------++--------------------------------||\n"\
"#                  ||              ||                         ||    Ʃ     |    μ     |     σ    ||    Ʃ     |     μ    |    σ     ||     Ʃ    |     μ    |     σ    ||\n"\
"# -----------------++--------------++-------------------------++----------+----------+----------++----------+----------+----------++----------+----------+----------||\n"

#define OCL_EXC_AWM_HEADER \
"##=========================================================================================================================================================================================================##\n" \
"## %100s-%-99d##\n" \
"##=========================================================================================================================================================================================================##\n"

#define OCL_STATS_BAR \
"#==========================================================================================================================================================================================================##\n"

#define OCL_STATS_BAR_ADDR \
"#===================================================================================================================================================================##\n"

void BbqueRPC::OclDumpStatsHeader(bool h) {
	if (h) {
		fprintf(outfd, OCL_STATS_BAR);
		fprintf(outfd, OCL_STATS_HEADER);
	}
	else {
		fprintf(outfd, OCL_STATS_BAR_ADDR);
		fprintf(outfd, OCL_STATS_HEADER_ADDR);
	}
}

void BbqueRPC::OclDumpStatsConsole(pregExCtx_t prec) {
	AwmStatsMap_t::iterator it;
	pAwmStats_t pstats;
	uint8_t awm_id;

	// Print RTLib stats for each AWM
	it = prec->stats.begin();
	for ( ; it != prec->stats.end(); ++it) {
		awm_id = (*it).first;
		pstats = (*it).second;
		std::map<cl_command_queue, QueueProfPtr_t>::iterator it_cq;
		fprintf(outfd, OCL_EXC_AWM_HEADER, prec->name.c_str(), awm_id);

		OclDumpStatsHeader(true);
		for (it_cq = pstats->ocl_events_map.begin(); it_cq != pstats->ocl_events_map.end(); it_cq++) {
			QueueProfPtr_t stPtr = it_cq->second;
			OclDumpCmdStatsConsole(stPtr, it_cq->first);
			if (envOCLProfLevel == 0)
				continue;
			OclDumpAddrStatsConsole(stPtr, it_cq->first);
		}
	}
	fprintf(outfd, "\n\n");
}

void BbqueRPC::OclDumpCmdStatsConsole(QueueProfPtr_t stPtr, cl_command_queue cmd_queue) {
	std::map<cl_command_type, AccArray_t>::iterator it_ct;
	double_t otot = 0, vtot_q = 0, vtot_s = 0, vtot_e = 0;
	for (it_ct = stPtr->cmd_prof.begin(); it_ct != stPtr->cmd_prof.end(); it_ct++) {
		vtot_q += SUM(QUEUED);
		vtot_s += SUM(SUBMIT);
		vtot_e += SUM(EXEC);
	}

	for (it_ct = stPtr->cmd_prof.begin(); it_ct != stPtr->cmd_prof.end(); it_ct++) {
		otot = SUM(QUEUED)+SUM(SUBMIT)+SUM(EXEC);
		fprintf(outfd, "# %-32p || %-23s || "
				"%7.3f ( %5.2f %5.2f ) | %8.3f | %8.3f || "
				"%7.3f ( %5.2f %5.2f ) | %8.3f | %8.3f || "
				"%7.3f ( %5.2f %5.2f ) | %8.3f | %8.3f ||\n",
			(void *) cmd_queue, ocl_cmd_str[it_ct->first].c_str(),
			SUM(QUEUED), (100 * SUM(QUEUED))/otot, (100 * SUM(QUEUED))/vtot_q,
			MEAN(QUEUED), STDDEV(QUEUED),
			SUM(SUBMIT), (100 * SUM(SUBMIT))/otot, (100 * SUM(SUBMIT))/vtot_s,
			MEAN(SUBMIT), STDDEV(SUBMIT),
			SUM(EXEC), (100 * SUM(EXEC))/otot, (100 * SUM(EXEC))/vtot_e,
			MEAN(EXEC), STDDEV(EXEC));
	}
	fprintf(outfd, OCL_STATS_BAR);
}
void BbqueRPC::OclDumpAddrStatsConsole(QueueProfPtr_t stPtr, cl_command_queue cmd_queue) {
	std::map<void *, AccArray_t>::iterator it_ct;
	cl_command_type cmd_type;

	OclDumpStatsHeader(false);
	for (it_ct = stPtr->addr_prof.begin(); it_ct != stPtr->addr_prof.end(); it_ct++) {
		cmd_type = rtlib_ocl_get_command_type(it_ct->first);

		fprintf(outfd, "# %-16p || %-12p || %-23s || "
				"%8.3f | %8.3f | %8.3f || "
				"%8.3f | %8.3f | %8.3f || "
				"%8.3f | %8.3f | %8.3f ||\n",
			(void *) cmd_queue, it_ct->first, ocl_cmd_str[cmd_type].c_str(),
			SUM(QUEUED), MEAN(QUEUED), STDDEV(QUEUED),
			SUM(SUBMIT), MEAN(SUBMIT), STDDEV(SUBMIT),
			SUM(EXEC), MEAN(EXEC), STDDEV(EXEC));
	}
	fprintf(outfd, OCL_STATS_BAR_ADDR);
}


#endif // CONFIG_BBQUE_OPENCL

/*******************************************************************************
 *    Utility Functions
 ******************************************************************************/

AppUid_t BbqueRPC::GetUid(RTLIB_ExecutionContextHandler_t ech) {
	pregExCtx_t prec;

	// Get a reference to the EXC to control
	assert(ech);
	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Unregister EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return RTLIB_EXC_NOT_REGISTERED;
	}
	assert(isRegistered(prec) == true);

	return ((chTrdPid << BBQUE_UID_SHIFT) + prec->exc_id);
}



/*******************************************************************************
 *    Cycles Per Second (CPS) Control Support
 ******************************************************************************/

RTLIB_ExitCode_t BbqueRPC::SetCPS(
	RTLIB_ExecutionContextHandler_t ech,
	float cps) {
	pregExCtx_t prec;

	// Get a reference to the EXC to control
	assert(ech);
	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Unregister EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return RTLIB_EXC_NOT_REGISTERED;
	}
	assert(isRegistered(prec) == true);

	// Keep track of the maximum required CPS
	prec->cps_max = cps;
	prec->cps_expect = 0;
	if (cps != 0) {
		prec->cps_expect = static_cast<float>(1e3) / prec->cps_max;
	}

	logger->Info("Set cycle-rate @ %.3f[Hz] (%.3f[ms])",
				prec->cps_max, prec->cps_expect);
	return RTLIB_OK;

}

void BbqueRPC::ForceCPS(pregExCtx_t prec) {
	float delay_ms = 0; // [ms] delay to stick with the required FPS
	uint32_t sleep_us;
	float cycle_time;
	double tnow; // [s] at the call time

	// Timing initialization
	if (unlikely(prec->cps_tstart == 0)) {
		// The first frame is used to setup the start time
		prec->cps_tstart = bbque_tmr.getElapsedTimeMs();
		return;
	}

	// Compute last cycle run time
	tnow = bbque_tmr.getElapsedTimeMs();
	logger->Debug("TP: %.4f, TN: %.4f", prec->cps_tstart, tnow);
	cycle_time = tnow - prec->cps_tstart;
	delay_ms = prec->cps_expect - cycle_time;

	// Enforce CPS if needed
	if (cycle_time < prec->cps_expect) {
		sleep_us = 1e3 * static_cast<uint32_t>(delay_ms);
		logger->Debug("Cycle Time: %3.3f[ms], ET: %3.3f[ms], Sleep time %u [us]",
					cycle_time, prec->cps_expect, sleep_us);
		usleep(sleep_us);
	}

	// Update the start time of the next cycle
	prec->cps_tstart = bbque_tmr.getElapsedTimeMs();
}

/*******************************************************************************
 *    RTLib Notifiers Support
 ******************************************************************************/

void BbqueRPC::NotifySetup(
	RTLIB_ExecutionContextHandler_t ech) {
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Unregister EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return;
	}

	assert(isRegistered(prec) == true);

	// Add all the required performance counters
	if (envPerfCount) {
		PerfSetupEvents(prec);
	}

}

void BbqueRPC::NotifyInit(
	RTLIB_ExecutionContextHandler_t ech) {
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("Unregister EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return;
	}

	assert(isRegistered(prec) == true);

	if (envGlobal && PerfRegisteredEvents(prec)) {
		PerfEnable(prec);
	}

}

void BbqueRPC::NotifyExit(
	RTLIB_ExecutionContextHandler_t ech) {
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("NotifyExit EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return;
	}

	assert(isRegistered(prec) == true);

	if (envGlobal && PerfRegisteredEvents(prec)) {
		PerfDisable(prec);
		PerfCollectStats(prec);
	}

}

void BbqueRPC::NotifyPreConfigure(
	RTLIB_ExecutionContextHandler_t ech) {
	logger->Debug("===> NotifyConfigure");
	(void)ech;
	pregExCtx_t prec;

	assert(ech);
	prec = getRegistered(ech);
	if (!prec) {
		fprintf(stderr, FE("NotifyPreConfigure EXC [%p] FAILED "
				"(EXC not registered)\n"), (void*)ech);
		return;
	}
	assert(isRegistered(prec) == true);
#ifdef CONFIG_BBQUE_OPENCL
	fprintf(stderr, FD("NotifyPreConfigure - OCL Device: %d\n"), prec->dev_id);
	OclSetDevice(prec->dev_id, prec->event);
#endif
}

void BbqueRPC::NotifyPostConfigure(
	RTLIB_ExecutionContextHandler_t ech) {
	pregExCtx_t prec;

	assert(ech);
	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("NotifyPostConfigure EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return;
	}
	assert(isRegistered(prec) == true);

	logger->Debug("<=== NotifyConfigure");

	// CPS Enforcing initialization
	if ((prec->cps_expect != 0) || (prec->cps_tstart == 0))
		prec->cps_tstart = bbque_tmr.getElapsedTimeMs();

	(void)ech;

#ifdef CONFIG_BBQUE_OPENCL
	// Clear pre-run OpenCL command events
	OclClearStats();
#endif
}

void BbqueRPC::NotifyPreRun(
	RTLIB_ExecutionContextHandler_t ech) {
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("NotifyPreRun EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return;
	}

	assert(isRegistered(prec) == true);

	logger->Debug("===> NotifyRun");
	if (!envGlobal && PerfRegisteredEvents(prec)) {
		if (unlikely(envOverheads)) {
			PerfDisable(prec);
			PerfCollectStats(prec);
		} else {
			PerfEnable(prec);
		}
	}
}

void BbqueRPC::NotifyPostRun(
	RTLIB_ExecutionContextHandler_t ech) {
	pregExCtx_t prec;

	assert(ech);

	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("NotifyPostRun EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return;
	}

	assert(isRegistered(prec) == true);

	logger->Debug("<=== NotifyRun");

	if (!envGlobal && PerfRegisteredEvents(prec)) {
		if (unlikely(envOverheads)) {
			PerfEnable(prec);
		} else {
			PerfDisable(prec);
			PerfCollectStats(prec);
		}
	}
#ifdef CONFIG_BBQUE_OPENCL
	if (envOCLProf)
		OclCollectStats(prec->awm_id, prec->pAwmStats->ocl_events_map);

#endif // CONFIG_BBQUE_OPENCL
}

void BbqueRPC::NotifyPreMonitor(
	RTLIB_ExecutionContextHandler_t ech) {
	pregExCtx_t prec;

	assert(ech);
	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("NotifyPostMonitor EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return;
	}
	assert(isRegistered(prec) == true);

	logger->Debug("===> NotifyMonitor");

	// Keep track of monitoring start time
	prec->mon_tstart = prec->exc_tmr.getElapsedTimeMs();
}

void BbqueRPC::NotifyPostMonitor(
	RTLIB_ExecutionContextHandler_t ech) {
	pregExCtx_t prec;

	assert(ech);
	prec = getRegistered(ech);
	if (!prec) {
		logger->Error("NotifyPostMonitor EXC [%p] FAILED "
				"(EXC not registered)", (void*)ech);
		return;
	}
	assert(isRegistered(prec) == true);

	logger->Debug("<=== NotifyMonitor");

	// Update monitoring statistics
	pAwmStats_t pstats = prec->pAwmStats;
	double last_monitor_ms = prec->exc_tmr.getElapsedTimeMs();
	last_monitor_ms -= prec->mon_tstart;

	pstats->time_monitoring += last_monitor_ms;
	pstats->monitor_samples(last_monitor_ms);

	// Statistic features extraction for cycle time estimation:
	DB(
	uint32_t _count = count(pstats->monitor_samples);
	double _min = min(pstats->monitor_samples);
	double _max = max(pstats->monitor_samples);
	double _avg = mean(pstats->monitor_samples);
	double _var = variance(pstats->monitor_samples);
	logger->Debug("Monitor #%08d: m: %.3f, M: %.3f, a: %.3f, v: %.3f) [ms]",
		_count, _min, _max, _avg, _var);
	)

	// CPS Enforcing
	if (prec->cps_expect != 0)
		ForceCPS(prec);


}

void BbqueRPC::NotifyPreSuspend(
	RTLIB_ExecutionContextHandler_t ech) {
	logger->Debug("===> NotifySuspend");
	(void)ech;
}

void BbqueRPC::NotifyPostSuspend(
	RTLIB_ExecutionContextHandler_t ech) {
	logger->Debug("<=== NotifySuspend");
	(void)ech;
}

void BbqueRPC::NotifyPreResume(
	RTLIB_ExecutionContextHandler_t ech) {
	logger->Debug("===> NotifyResume");
	(void)ech;
}

void BbqueRPC::NotifyPostResume(
	RTLIB_ExecutionContextHandler_t ech) {
	logger->Debug("<=== NotifyResume");
	(void)ech;
}

void BbqueRPC::NotifyRelease(
	RTLIB_ExecutionContextHandler_t ech) {
	logger->Debug("===> NotifyRelease");
	(void)ech;
}

} // namespace rtlib

} // namespace bbque

