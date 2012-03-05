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
#include "bbque/system_view.h"
#include "bbque/plugins/scheduler_policy.h"
#include "bbque/plugins/plugin.h"

#include "contrib/metrics_contribute.h"

#define SCHEDULER_POLICY_NAME "yams"

/** Metrics (class SAMPLE) declaration */
#define YAMS_SAMPLE_METRIC(NAME, DESC)\
 {SCHEDULER_MANAGER_NAMESPACE".yams."NAME, DESC, MetricsCollector::SAMPLE, 0}
/** Reset the timer used to evaluate metrics */
#define YAMS_RESET_TIMING(TIMER) \
	TIMER.start();
/** Acquire a new completion time sample */
#define YAMS_GET_TIMING(METRICS, INDEX, TIMER) \
	mc.AddSample(METRICS[INDEX].mh, TIMER.getElapsedTimeMs());

/* Get a new samplefor the metrics */
#define YAMS_GET_SAMPLE(METRICS, INDEX, VALUE) \
	mc.AddSample(METRICS[INDEX].mh, VALUE);

using namespace bbque::app;
using namespace bbque::res;

using bbque::app::AppPrio_t;
using bbque::app::AppCPtr_t;
using bbque::app::AwmPtr_t;
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
class YamsSchedPol: public SchedulerPolicyIF {

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
	~YamsSchedPol();

	/**
	 * @see SchedulerPolicyIF
	 */
	char const * Name();

	/**
	 * @see SchedulerPolicyIF
	 */
	ExitCode_t Schedule(SystemView & sview, RViewToken_t & rav);

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
	 * @brief Types of metrics contributes
	 */
	enum Contributes_t {
		YAMS_VALUE = 0,
		YAMS_RECONFIG,
		YAMS_CONGESTION,
		YAMS_FAIRNESS,
		//YAMS_POWER,
		//YAMS_THERMAL,
		//YAMS_STABILITY,
		//YAMS_ROBUSTNESS,
		// ...:: ADD_MCT ::...

		YAMS_MCT_COUNT
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

	/**
	 * @brief Scheduling entity
	 *
	 * This embodies all the information needed in the "selection" step to require
	 * a scheduling for an application into a specific AWM, with the resource set
	 * bound into a chosen cluster
	 */
	struct SchedEntity_t: public EvalEntity_t {

		SchedEntity_t(AppCPtr_t _papp, AwmPtr_t _pawm, uint8_t _clid,
				float _metr):
			EvalEntity_t(_papp, _pawm, _clid),
			metrics(_metr) {
			};

		/** Metrics computed */
		float metrics;
	};

	/** Shared pointer to a scheduling entity */
	typedef std::shared_ptr<SchedEntity_t> SchedEntityPtr_t;

	/** List of scheduling entities */
	typedef std::list<SchedEntityPtr_t> SchedEntityList_t;

	/** Shared pointer to a metrics contribute */
	typedef std::shared_ptr<MetricsContribute> MetricsContribPtr_t;


	/** Configuration manager instance */
	ConfigurationManager & cm;

	/** Resource accounter instance */
	ResourceAccounter & ra;

	/** Metric collector instance */
	MetricsCollector & mc;

	/** System logger instance */
	LoggerIF *logger;

	/** System view instance */
	SystemView * sv;

	/** Token for accessing a resources view */
	RViewToken_t vtok;

	/** A counter used for getting always a new clean resources view */
	uint32_t vtok_count;

	/** List of entities to schedule */
	SchedEntityList_t entities;


	/** Metrics contribute instances*/
	MetricsContribPtr_t mcts[YAMS_MCT_COUNT];

	/** Metrics contribute configuration keys */
	static char const * mct_str[YAMS_MCT_COUNT];

	/** Metrics contributes weights */
	static uint16_t mct_weights[YAMS_MCT_COUNT];

	/** Normalized metrics contributes weights */
	float mct_weights_norm[YAMS_MCT_COUNT];

	/** Global config parameters for metrics contributes */
	static uint16_t mct_cfg_params[MetricsContribute::MCT_CPT_COUNT];


	/**
	 * @brief ClustersInfo
	 *
	 * Keep track of the runtime status of the clusters
	 */
	struct ClustersInfo_t {
		/** Number of clusters on the platform	 */
		uint16_t num;
		/** Resource pointer descriptor list */
		ResourcePtrList_t rsrcs;
		/** The IDs of all the available clusters */
		std::vector<ResID_t> ids;
		/** Keep track the clusters without available PEs */
		ClustersBitSet full;
	} cl_info;

	/** Mutex */
	std::mutex sched_mtx;


	/** The High-Resolution timer used for profiling */
	Timer yams_tmr;

	/** Statistical metrics of the scheduling policy */
	static MetricsCollector::MetricsCollection_t
		coll_metrics[YAMS_METRICS_COUNT];

	/** Statistical metrics for single contributes */
	static MetricsCollector::MetricsCollection_t
		coll_mct_metrics[YAMS_MCT_COUNT];


	/**
	 * @brief The plugins constructor
	 *
	 * Plugins objects could be build only by using the "create" method.
	 * Usually the PluginManager acts as object
	 */
	YamsSchedPol();

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
	 * @brief Normalize metrics contribute weights
	 */
	void NormalizeMCTWeights();

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
	 * @param cl_id The current cluster for the clustered resources
	 */
	void OrderSchedEntities(AppPrio_t prio,	uint16_t cl_id);

	/**
	 * @brief Metrics of all the AWMs of an Application
	 *
	 * @param papp Shared pointer to the Application/EXC to schedule
	 * @param cl_id The current cluster for the clustered resources
	 */
	 void InsertWorkingModes(AppCPtr_t const & papp, uint16_t cl_id);

	/**
	 * @brief Evaluate an AWM
	 *
	 * @param pschd The scheduling entity to evaluate
	 */
	void EvalWorkingMode(SchedEntityPtr_t pschd);

	/**
	 * @brief Require the scheduling of the entities
	 *
	 * For each application pick the next working mode to schedule
	 */
	void SelectSchedEntities();

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
			logger->Debug("Skipping [%s]. State = {%s/%s}",
					papp->StrId(),
					ApplicationStatusIF::StateStr(papp->State()),
					ApplicationStatusIF::SyncStateStr(papp->SyncState()));
			return true;
		}

		// Avoid double AWM selection for RUNNING applications with an already
		// assigned AWM.
		if ((papp->State() == Application::RUNNING) && papp->NextAWM()) {
			logger->Debug("Skipping [%s]. No reconfiguration needed. (AWM=%d)",
					papp->StrId(), papp->CurrentAWM()->Id());
			return true;
		}

		return false;
	}

	/**
	 * @brief Bind the resources of the AWM into a given cluster
	 *
	 * @param pschd The scheduling entity to evaluate
	 *
	 * @return YAMS_SUCCESS for success, YAMS_ERROR if an unexpected error has
	 * been encountered
	 */
	YamsSchedPol::ExitCode_t BindCluster(SchedEntityPtr_t pschd);

	/**
	 * @brief Compute the metrics of the given scheduling entity
	 *
	 * This puts together all the contributes for the metrics computation
	 *
	 * @param pschd The scheduling entity to evaluate
	 */
	void AggregateContributes(SchedEntityPtr_t pschd);

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

