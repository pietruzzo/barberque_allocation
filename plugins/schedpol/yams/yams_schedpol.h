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

#ifndef BBQUE_YAMS_SCHEDPOL_H_
#define BBQUE_YAMS_SCHEDPOL_H_

#include <cstdint>

#include "bbque/configuration_manager.h"
#include "bbque/scheduler_manager.h"
#include "bbque/command_manager.h"
#include "bbque/plugins/plugin.h"

#include "contrib/sched_contrib_manager.h"

#undef  MODULE_NAMESPACE
#undef  MODULE_CONFIG
#define SCHEDULER_POLICY_NAME "yams"
#define MODULE_NAMESPACE SCHEDULER_POLICY_NAMESPACE "." SCHEDULER_POLICY_NAME
#define MODULE_CONFIG SCHEDULER_POLICY_CONFIG "." SCHEDULER_POLICY_NAME

/** Metrics (class SAMPLE) declaration */
#define YAMS_SAMPLE_METRIC(NAME, DESC)\
 {SCHEDULER_MANAGER_NAMESPACE ".yams." NAME, DESC, \
	 MetricsCollector::SAMPLE, 0, NULL, 0}
/** Reset the timer used to evaluate metrics */
#define YAMS_RESET_TIMING(TIMER) \
	TIMER.start();
/** Acquire a new completion time sample */
#define YAMS_GET_TIMING(METRICS, INDEX, TIMER) \
	mc.AddSample(METRICS[INDEX].mh, TIMER.getElapsedTimeMs());

/* Get a new sample for the metrics */
#define YAMS_GET_SAMPLE(METRICS, INDEX, VALUE) \
	mc.AddSample(METRICS[INDEX].mh, VALUE);

/* Number of scheduling contributions used for the scheduling metrics
 * computation */

#define YAMS_AWM_SC_COUNT 3
#ifndef CONFIG_BBQUE_SP_COWS_BINDING
#define YAMS_BD_SC_COUNT  2
#else
#define YAMS_BD_SC_COUNT  1
#endif
#define YAMS_SC_COUNT (YAMS_AWM_SC_COUNT + YAMS_BD_SC_COUNT)

#ifdef CONFIG_BBQUE_SP_COWS_BINDING
#define COWS_BOUNDNESS_METRICS 1
#define COWS_SYSWIDE_METRICS 3
#define COWS_ADDITIONAL_METRICS 1
#define COWS_RECIPE_METRICS (COWS_BOUNDNESS_METRICS + COWS_SYSWIDE_METRICS)
#define COWS_NORMALIZATION_VALUES (COWS_RECIPE_METRICS + COWS_ADDITIONAL_METRICS)
#endif

using bbque::res::RViewToken_t;
using bbque::utils::Timer;
using bbque::utils::MetricsCollector;


// These are the parameters received by the PluginManager on create calls
struct PF_ObjectParams;

namespace bbque { namespace plugins {


// Forward declaration
class LoggerIF;

/**
 * @class YamsSchedPol
 *
 * The yams resource scheduler heuristic registered as a dynamic C++
 * plugin.
 */
class YamsSchedPol: public SchedulerPolicyIF, CommandHandler {

public:

	// :::::::::::::::::::::: Static plugin interface :::::::::::::::::::::::::

	/**
	 * @brief Create the plugin
	 */
	static void * Create(PF_ObjectParams *);

	/**
	 * @brief Destroy the plugin
	 */
	static int32_t Destroy(void *);


	// :::::::::::::::::: Scheduler policy module interface :::::::::::::::::::

	/**
	 * @brief Destructor
	 */
	virtual ~YamsSchedPol();

	/**
	 * @see SchedulerPolicyIF
	 */
	char const * Name();

	/**
	 * @see SchedulerPolicyIF
	 */
	ExitCode_t Schedule(System & sys_if, RViewToken_t & rav);

	/**
	 * @see CommandHandler
	 */
	int CommandsCb(int argc, char *argv[]);


private:

	/**
	 * @brief Specific internal exit code for YaMS
	 */
	enum ExitCode_t {
		YAMS_SUCCESS,
		YAMS_CLUSTER_FULL,
		YAMS_ERROR,
		YAMS_ERR_VIEW,
		YAMS_ERR_CLUSTERS
	};

	/**
	 * @brief Collection of statistical metrics generated by this module
	 */
	enum SchedPolMetrics_t {
		// :::: Timing metrics
		YAMS_ORDERING_TIME,
		YAMS_SELECTING_TIME,
		YAMS_METRICS_COMP_TIME,
		YAMS_METRICS_AWMVALUE,

		YAMS_METRICS_COUNT
	};

	/**
	 * @brief Collection of statistical metrics for contribute computations
	 */
	enum MCTimeMetrics_t {
		YAMS_VALUE_COMP_TIME,
		YAMS_RECONFIG_COMP_TIME,
		YAMS_CONGESTION_COMP_TIME,
		YAMS_FAIRNESS_COMP_TIME
		// ...:: ADD_MCT ::...
	};


	/** Shared pointer to a scheduling entity */
	typedef std::shared_ptr<SchedEntity_t> SchedEntityPtr_t;

	/** List of scheduling entities */
	typedef std::list<SchedEntityPtr_t> SchedEntityList_t;


	/** Configuration manager instance */
	ConfigurationManager & cm;

	/** Resource accounter instance */
	ResourceAccounter & ra;

	/** Metric collector instance */
	MetricsCollector & mc;

	/** Command manager instance */
	CommandManager &cmm;

	/** System logger instance */
	LoggerIF *logger = NULL;

	/** System view instance */
	System * sv = NULL;

	/** Token for accessing a resources view */
	RViewToken_t vtok = 0;

	/** A counter used for getting always a new clean resources view */
	uint32_t vtok_count = 0;

	/** List of entities to schedule */
	SchedEntityList_t entities;

	/** Manager for the scheduling contributions set */
	SchedContribManager * scm;

	/** Set of scheduling contributions type used for the metrics */
	static SchedContribManager::Type_t sc_types[YAMS_SC_COUNT];

#ifdef CONFIG_BBQUE_SP_COWS_BINDING

	/**Cows metrics*/
	struct cowsSystemInfo{
		/** Total value and squared value of boundness for each BD */
		std::vector<float> boundnessSquaredSum;
		std::vector<float> boundnessSum;
		/**Total stalls, retired instr. and flops for each BD */
		std::vector<float> stallsSum;
		std::vector<float> retiredSum;
		std::vector<float> flopSum;
		/**Applications scheduled on each BD and on the whole system */
		std::vector<int> bdLoad;
		int bdTotalLoad;
		/*Metrics of the SchedEnt under analysis */
		std::vector<float> candidatedValues;
		std::vector<int> candidatedBindings;
		/*statistic info used to normalize the metrics */
		std::vector<float> normStats;
		/*Total metrics allocated on the system during AWM analysis */
		std::vector<float> modifiedSums;
		/* Metrics container per BD */
		std::vector<float> boundnessMetrics;
		std::vector<float> stallsMetrics;
		std::vector<float> retiredMetrics;
		std::vector<float> flopsMetrics;
		std::vector<float> migrationMetrics;
		/* Weights used to compute metrics */
		std::vector<float> metricsWeights;
	} cowsInfo;

#endif

	/** Collect information on binding domains */
	BindingInfo_t bindings;


	/** Mutex */
	std::mutex sched_mtx;


	/** The High-Resolution timer used for profiling */
	Timer yams_tmr;

	/** Statistical metrics of the scheduling policy */
	static MetricsCollector::MetricsCollection_t
		coll_metrics[YAMS_METRICS_COUNT];

	/** Statistical metrics for single contributes */
	static MetricsCollector::MetricsCollection_t
		coll_mct_metrics[YAMS_SC_COUNT];


	/**
	 * @brief The plugins constructor
	 *
	 * Plugins objects could be build only by using the "create" method.
	 * Usually the PluginManager acts as object
	 */
	YamsSchedPol();

	/**
	 * @brief Clear information on scheduling entitties and bindings
	 */
	void Clear();

	/**
	 * @brief Perform initialization operation
	 *
	 * In particular the method gets a token for accessing a clean resource
	 * state view, and retrieve the number of clusters available on the
	 * platform.
	 *
	 * @return YAMS_ERR_VIEW if a resource state view cannot be retrieved.
	 * YAMS_ERR_CLUSTER if no clusters have been found on the platform.
	 * YAMS_SUCCESS if initialization has been successfyully completed
	 */
	YamsSchedPol::ExitCode_t Init();

	/**
	 * @brief Initialize a new resource state view
	 */
	YamsSchedPol::ExitCode_t InitResourceStateView();

	/**
	 * @brief Initialize the base information needed for the resource binding
	 */
	YamsSchedPol::ExitCode_t InitBindingInfo();

	/**
	 * @brief Initalize scheduling contributions managers (one per binding
	 * domain)
	 */
	YamsSchedPol::ExitCode_t InitSchedContribManagers();

	/**
	 * @brief Schedule applications from a priority queue
	 *
	 * @param prio The priority applications queue to schedule
	 */
	void SchedulePrioQueue(AppPrio_t prio);

	/**
	 * @brief Order the scheduling entities per metrics value
	 *
	 * For each application create a scheduling entity made by the tern
	 * {Application, WorkingMode, Cluster ID} and place it into a list
	 * which is sorted in descending order of metrics value
	 *
	 * @param prio The priority queue to schedule
	 *
	 * @return the number of EXCs in this priority queue which have a
	 * NAP value asserted
	 */
	uint8_t OrderSchedEntities(AppPrio_t prio);

	/**
	 * @brief Metrics of all the AWMs of an Application
	 *
	 * @param papp Shared pointer to the Application/EXC to schedule
	 */
	void InsertWorkingModes(AppCPtr_t const & papp);

	/**
	 * @brief Evaluate an AWM
	 *
	 * @param pschd The scheduling entity to evaluate
	 */
	void EvalWorkingMode(SchedEntityPtr_t pschd);

	/**
	 * @brief Compute the metrics of the given scheduling entity
	 *
	 * This puts together all the contributes for the metrics computation
	 *
	 * @param pschd The scheduling entity to evaluate
	 */
	void GetSchedContribValue(SchedEntityPtr_t pschd,
			SchedContribManager::Type_t sc_type, float & sc_value);

	/**
	 * @brief Evaluate an AWM in a specific binding domain
	 *
	 * @param pschd The scheduling entity to evaluate
	 */
	ExitCode_t EvalBinding(SchedEntityPtr_t pschd_bd, float & value);

	/**
	 * @brief Bind the resources of the AWM into the given binding domain
	 *
	 * @param pschd The scheduling entity to evaluate
	 *
	 * @return YAMS_SUCCESS for success, YAMS_ERROR if an unexpected error has
	 * been encountered
	 */
	ExitCode_t BindResources(SchedEntityPtr_t pschd);

	/**
	 * @brief COWS: Evaluate a Binding
	 *
	 * @param pschd The scheduling entity to evaluate
	 */
	void CowsSetOptBinding(SchedEntityPtr_t pschd);

	/**
	 * @brief COWS: reset resource counters
	 *
	 */
	void CowsResetStatus();

	/**
	 * @brief COWS: apply values extracted from recipe
	 *
	 * @param db Cache misses per cycle of the current AWM
	 * @param ds Stalls per cycle of the current AWM
	 * @param dr Retired instructions per cycle of the current AWM
	 */
	void CowsApplyRecipeValues(float db, float ds, float dr, float df);

	/**
	 * @brief COWS: compute stalls, retired, flops var delta for the system
	 *
	 */
	void CowsSysWideMetrics();

	/**
	 * @brief COWS: compute the boundness variance variation for all BDs
	 *
	 * @param boundness Container for the boundness metrics
	 * @param percentualBoundnessDenom Needed to normalize the metrics
	 * @param psch The scheduling entity to use
         *
	 */
	void CowsComputeBoundness(SchedEntityPtr_t psch);

	/**
	 * @brief COWS: aggregate results
	 *
	 * @param psch The scheduling entity to use
         *
	 */
	void CowsAggregateResults(SchedEntityPtr_t psch);

	/**
	 * @brief COWS: Update Means
	 *
	 * @param pschd The scheduling entity to use to update
	 */
	void CowsUpdateMeans(int logic_index);
	void CowsUpdateMeans(SchedEntityPtr_t pschd);

	/**
	 * @brief Require the scheduling of the entities
	 *
	 * For each application pick the next working mode to schedule.
	 * If a number of applications with NAP asserted has been specified,
	 * than this method return (with a true) as soon as all the "NAPped"
	 * applications have been already scheduled.
	 *
	 * @param naps_count the number of EXC, among those to be selected,
	 * which have a NAP value asserted.
	 * @return true if the schedule for these entities has not been
	 * competed
	 */
	bool SelectSchedEntities(uint8_t naps_count);

	/**
	 * @brief Check if an application/EXC must be skipped
	 *
	 * Whenever we are in the ordering or the selecting step, the
	 * application/EXC must be skipped if some conditions are verified
	 *
	 * @param papp Shared pointer to the Application/EXC to schedule
	 * @return true if the Application/EXC must be skipped, false otherwise
	 */
	inline bool CheckSkipConditions(AppCPtr_t const & papp) {
		// Skip if the application has been rescheduled yet (with success) or
		// disabled in the meanwhile
		if (!papp->Active() && !papp->Blocking()) {
			logger->Debug("Skipping [%s] State = [%s, %s]",
					papp->StrId(),
					ApplicationStatusIF::StateStr(papp->State()),
					ApplicationStatusIF::SyncStateStr(papp->SyncState()));
			return true;
		}

		// Avoid double AWM selection for RUNNING applications with already
		// assigned AWM.
		if ((papp->State() == Application::RUNNING) && papp->NextAWM()) {
			logger->Debug("Skipping [%s] AWM %d => No reconfiguration",
					papp->StrId(), papp->CurrentAWM()->Id());
			return true;
		}

		// Avoid double AWM selection for SYNCH applications with an already
		// assigned AWM.
		if ((papp->State() == Application::SYNC) && papp->NextAWM()) {
			logger->Debug("Skipping [%s] AWM already assigned [%d]",
					papp->StrId(), papp->NextAWM()->Id());
			return true;
		}

		return false;
	}

	/**
	 * @brief Compare scheduling entities
	 *
	 * The function is used to order the list of scheduling entities
	 */
	static bool CompareEntities(SchedEntityPtr_t & se1,
			SchedEntityPtr_t & se2);

};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_YAMS_SCHEDPOL_H_

