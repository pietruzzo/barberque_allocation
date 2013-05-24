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

#include "yams_schedpol.h"

#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <functional>
#include "bbque/cpp11/thread.h"
#include "bbque/modules_factory.h"
#include "bbque/app/working_mode.h"
#include "bbque/plugins/logger.h"
#include "contrib/sched_contrib_manager.h"

namespace bu = bbque::utils;
namespace po = boost::program_options;

namespace bbque { namespace plugins {


SchedContribManager::Type_t YamsSchedPol::sc_types[] = {
	SchedContribManager::VALUE,
	SchedContribManager::RECONFIG,
	SchedContribManager::FAIRNESS,
#ifndef CONFIG_BBQUE_SP_COWS_BINDING
	SchedContribManager::CONGESTION,
	SchedContribManager::MIGRATION
#endif
};

// Definition of time metrics of the scheduling policy
MetricsCollector::MetricsCollection_t
YamsSchedPol::coll_metrics[YAMS_METRICS_COUNT] = {
	YAMS_SAMPLE_METRIC("ord",
			"Time to order SchedEntity into a cluster [ms]"),
	YAMS_SAMPLE_METRIC("sel",
			"Time to select AWMs/Clusters for the EXC [ms]"),
	YAMS_SAMPLE_METRIC("mcomp",
			"Time for computing a single metrics [ms]"),
	YAMS_SAMPLE_METRIC("awmvalue",
			"AWM value of the scheduled entity")
};

// Definition of time metrics for each SchedContrib computation
MetricsCollector::MetricsCollection_t
YamsSchedPol::coll_mct_metrics[YAMS_SC_COUNT] = {
	YAMS_SAMPLE_METRIC("awmv.comp",
			"AWM value computing time [ms]"),
	YAMS_SAMPLE_METRIC("recf.comp",
			"Reconfiguration contribution computing time [ms]"),
	YAMS_SAMPLE_METRIC("fair.comp",
			"Fairness contribution computing time [ms]"),
#ifndef CONFIG_BBQUE_SP_COWS_BINDING
	YAMS_SAMPLE_METRIC("cgst.comp",
			"Congestion contribution computing time [ms]"),
	YAMS_SAMPLE_METRIC("migr.comp",
			"Migration contribution computing time [ms]")
#endif
	// ...:: ADD_MCT ::...
};

// :::::::::::::::::::::: Static plugin interface ::::::::::::::::::::::::::::

void * YamsSchedPol::Create(PF_ObjectParams *) {
	return new YamsSchedPol();
}

int32_t YamsSchedPol::Destroy(void * plugin) {
	if (!plugin)
		return -1;
	delete (YamsSchedPol *)plugin;
	return 0;
}

// ::::::::::::::::::::: Scheduler policy module interface :::::::::::::::::::

char const * YamsSchedPol::Name() {
	return SCHEDULER_POLICY_NAME;
}

YamsSchedPol::YamsSchedPol():
	cm(ConfigurationManager::GetInstance()),
	ra(ResourceAccounter::GetInstance()),
	mc(bu::MetricsCollector::GetInstance()) {

	// Get a logger
	plugins::LoggerIF::Configuration conf(MODULE_NAMESPACE);
	logger = ModulesFactory::GetLoggerModule(std::cref(conf));
	if (logger)
		logger->Info("yams: Built a new dynamic object[%p]", this);
	else
		fprintf(stderr, FI("yams: Built new dynamic object [%p]\n"), (void *)this);

	// Binding domain resource path
	po::options_description opts_desc("Scheduling policy parameters");
	opts_desc.add_options()
		(SCHEDULER_POLICY_CONFIG".binding.domain",
		 po::value<std::string>
		 (&bindings.domain)->default_value(SCHEDULER_DEFAULT_BINDING_DOMAIN),
		"Resource binding domain");
	;
	po::variables_map opts_vm;
	cm.ParseConfigurationFile(opts_desc, opts_vm);

	// Binding domain resource type
	ResourcePath rp(bindings.domain);
	bindings.type = rp.Type();
	logger->Debug("Binding domain:'%s' Type:%s",
			bindings.domain.c_str(),
			ResourceIdentifier::TypeStr[bindings.type]);

	// Instantiate the SchedContribManager
	scm = new SchedContribManager(sc_types, bindings, YAMS_SC_COUNT);

	// Resource view counter
	vtok_count = 0;

	// Register all the metrics to collect
	mc.Register(coll_metrics, YAMS_METRICS_COUNT);
	mc.Register(coll_mct_metrics, YAMS_SC_COUNT);
}

YamsSchedPol::~YamsSchedPol() {

}

YamsSchedPol::ExitCode_t YamsSchedPol::Init() {
	ExitCode_t result;

	// Init a new resource state view
	result = InitResourceStateView();
	if (result != YAMS_SUCCESS)
		return result;

	// Init resource bindings information
	InitBindingInfo();

	// Initialize information for scheduling contributions
	InitSchedContribManagers();

	return YAMS_SUCCESS;
}

YamsSchedPol::ExitCode_t YamsSchedPol::InitResourceStateView() {
	ResourceAccounterStatusIF::ExitCode_t ra_result;
	char token_path[30];

	// Set the resource state view token counter
	++vtok_count;

	// Build a string path for the resource state view
	snprintf(token_path, 30, "%s%d", MODULE_NAMESPACE, vtok_count);
	logger->Debug("Init: Lowest application prio : %d",
			sv->ApplicationLowestPriority());

	// Get a resource state view
	logger->Debug("Init: Requiring state view token for %s", token_path);
	ra_result = ra.GetView(token_path, vtok);
	if (ra_result != ResourceAccounterStatusIF::RA_SUCCESS) {
		logger->Fatal("Init: Cannot get a resource state view");
		return YAMS_ERR_VIEW;
	}
	logger->Debug("Init: Resources state view token = %d", vtok);

	return YAMS_SUCCESS;
}

YamsSchedPol::ExitCode_t YamsSchedPol::InitBindingInfo() {
	bindings.rsrcs = sv->GetResources(bindings.domain);
	bindings.num   = bindings.rsrcs.size();
	bindings.ids.resize(bindings.num);
	if (bindings.num == 0) {
		logger->Error("Init: No bindings to R{%s} possible. "
				"Please check the platform configuration.",
				bindings.domain.c_str());
		return YAMS_ERR_CLUSTERS;
	}

	// Get all the possible resource binding IDs
	ResourcePtrList_t::iterator b_it(bindings.rsrcs.begin());
	ResourcePtrList_t::iterator b_end(bindings.rsrcs.end());
	for (uint8_t j = 0; b_it != b_end; ++b_it, ++j) {
		ResourcePtr_t const & rsrc(*b_it);
		bindings.ids[j] = rsrc->ID();
		logger->Debug("Init: R{%s} ID: %d",
				bindings.domain.c_str(), bindings.ids[j]);
	}
	logger->Debug("Init: R{%s}: %d possible bindings",
			bindings.domain.c_str(), bindings.num);

	return YAMS_SUCCESS;
}

YamsSchedPol::ExitCode_t YamsSchedPol::InitSchedContribManagers() {
	SchedContribPtr_t sc_recf;

#ifdef CONFIG_BBQUE_SP_COWS_BINDING
	// COWS: Vectors resizing depending on the number of binding domains
	cowsInfo.boundnessSquaredSum.resize(bindings.num);
	cowsInfo.boundnessSum.resize(bindings.num);
	cowsInfo.stallsSum.resize(bindings.num);
	cowsInfo.retiredSum.resize(bindings.num);
	cowsInfo.flopSum.resize(bindings.num);
	cowsInfo.bdLoad.resize(bindings.num);
	cowsInfo.boundnessMetrics.resize(bindings.num);
	cowsInfo.stallsMetrics.resize(bindings.num);
	cowsInfo.retiredMetrics.resize(bindings.num);
	cowsInfo.flopsMetrics.resize(bindings.num);
	cowsInfo.migrationMetrics.resize(bindings.num);
	cowsInfo.candidatedValues.resize(4);
	cowsInfo.normStats.resize(5);
	cowsInfo.modifiedSums.resize(3);

	// COWS: Reset the counters
	CowsResetStatus();
	logger->Info("COWS: Support enabled");
#endif

	// Set the view information into the metrics contribute
	scm->SetViewInfo(sv, vtok);
	scm->SetBindingInfo(bindings);
	logger->Debug("Init: SchedContribs: %d", YAMS_SC_COUNT);

	// Init Reconfig contribution
	ResID_t first_id = *(bindings.ids.begin());
	sc_recf = scm->GetContrib(SchedContribManager::RECONFIG);
	if (sc_recf == nullptr) {
		logger->Error("Init: SchedContrib 'RECONFIG' missing");
		return YAMS_ERROR;
	}
	assert(sc_recf != nullptr);
	sc_recf->Init(&first_id);

	return YAMS_SUCCESS;
}

SchedulerPolicyIF::ExitCode_t
YamsSchedPol::Schedule(System & sys_if, RViewToken_t & rav) {
	ExitCode_t result;

	// Save a reference to the System interface;
	sv = &sys_if;

	// Initialize a new resources state view
	result = Init();
	if (result != YAMS_SUCCESS)
		goto error;

	// Schedule per priority
	for (AppPrio_t prio = 0; prio <= sv->ApplicationLowestPriority(); ++prio) {
		if (!sv->HasApplications(prio))
			continue;
		SchedulePrioQueue(prio);
	}

	// Set the new resource state view token
	rav = vtok;

	// Reset scheduling entities and resource bindings status
	Clear();
	bindings.full.Reset();

	// Report table
	ra.PrintStatusReport(vtok);
	return SCHED_DONE;

error:
	logger->Error("Schedule: an error occurred. Interrupted.");
	bindings.full.Reset();
	Clear();
	ra.PutView(vtok);
	return SCHED_ERROR;
}

inline void YamsSchedPol::Clear() {
	entities.clear();
}
void YamsSchedPol::SchedulePrioQueue(AppPrio_t prio) {
	bool sched_incomplete;
	uint8_t naps_count = 0;
	SchedContribPtr_t sc_fair;

	// Init Fairness contribution
	sc_fair = scm->GetContrib(SchedContribManager::FAIRNESS);
	assert(sc_fair != nullptr);
	sc_fair->Init(&prio);

	// Reset timer
	YAMS_RESET_TIMING(yams_tmr);

do_schedule:
	// Order schedule entities by aggregate metrics
	naps_count = OrderSchedEntities(prio);
	YAMS_GET_TIMING(coll_metrics, YAMS_ORDERING_TIME, yams_tmr);
	YAMS_RESET_TIMING(yams_tmr);

	// Select and schedule the best bound AWM for each application
	sched_incomplete = SelectSchedEntities(naps_count);
	entities.clear();
	if (sched_incomplete)
		goto do_schedule;

	// Stop timing metrics
	YAMS_GET_TIMING(coll_metrics, YAMS_SELECTING_TIME, yams_tmr);
}

uint8_t YamsSchedPol::OrderSchedEntities(AppPrio_t prio) {
	uint8_t naps_count = 0;
	AppsUidMapIt app_it;
	AppCPtr_t papp;

	// Applications to be scheduled
	papp = sv->GetFirstWithPrio(prio, app_it);
	for (; papp; papp = sv->GetNextWithPrio(prio, app_it)) {
		// Check if the Application/EXC must be skipped
		if (CheckSkipConditions(papp))
			continue;

		// Compute the metrics for each AWM binding resources to cluster 'bd_id'
		InsertWorkingModes(papp);

		// Keep track of NAPped Applications/EXC
		if (papp->GetGoalGap())
			++naps_count;
	}

	// Order the scheduling entities list
	entities.sort(CompareEntities);

	return naps_count;
}

bool YamsSchedPol::SelectSchedEntities(uint8_t naps_count) {
	Application::ExitCode_t app_result;
	SchedEntityList_t::iterator se_it(entities.begin());
	SchedEntityList_t::iterator end_se(entities.end());
	logger->Debug("=================| Scheduling entities |=================");

	// Pick the entity and set the new AWM
	for (; se_it != end_se; ++se_it) {
		SchedEntityPtr_t & pschd(*se_it);

		// Skip if the Application/EXC has been already scheduled
		if (CheckSkipConditions(pschd->papp))
			continue;

#ifdef CONFIG_BBQUE_SP_COWS_BINDING
		// COWS: Find the best binding for the AWM of the Application
		CowsSetOptBinding(pschd);
#endif
		// Send the schedule request
		app_result = pschd->papp->ScheduleRequest(
				pschd->pawm, vtok, pschd->bind_id);
		logger->Debug("Selecting: %s schedule requested", pschd->StrId());
		if (app_result != ApplicationStatusIF::APP_WM_ACCEPTED) {
			logger->Debug("Selecting: %s rejected!", pschd->StrId());
			continue;
		}
#ifdef CONFIG_BBQUE_SP_COWS_BINDING
		//COWS: Update means and square means values
		CowsUpdateMeans(pschd);
#endif
		if (!pschd->papp->Synching() || pschd->papp->Blocking()) {
			logger->Debug("Selecting: [%s] state %s|%s", pschd->papp->StrId(),
					Application::StateStr(pschd->papp->State()),
					Application::SyncStateStr(pschd->papp->SyncState()));
			continue;
		}
		logger->Notice("Selecting: %s SCHEDULED metrics: %.4f",
				pschd->StrId(), pschd->metrics);

		// Set the application value (scheduling metrics)
		pschd->papp->SetValue(pschd->metrics);
		YAMS_GET_SAMPLE(coll_metrics, YAMS_METRICS_AWMVALUE,
				pschd->pawm->Value());

		// Break as soon as all NAPped apps have been scheduled
		if (naps_count && (--naps_count == 0))
			break;
	}

	if (se_it != end_se) {
		logger->Debug("======================| NAP Break |===================");
		return true;
	}

	logger->Debug("========================| DONE |======================");
	return false;
}

void YamsSchedPol::InsertWorkingModes(AppCPtr_t const & papp) {
	std::list<std::thread> awm_thds;
	AwmPtrList_t const * awms = nullptr;
	AwmPtrList_t::const_iterator awm_it;

	// AWMs evaluation (no binding)
	awms = papp->WorkingModes();
	for (awm_it = awms->begin(); awm_it != awms->end(); ++awm_it) {
		AwmPtr_t const & pawm(*awm_it);
		SchedEntityPtr_t pschd(new SchedEntity_t(papp, pawm, R_ID_NONE, 0.0));
#ifdef CONFIG_BBQUE_SP_YAMS_PARALLEL
		awm_thds.push_back(
				std::thread(&YamsSchedPol::EvalWorkingMode, this, pschd)
		);
#else
		EvalWorkingMode(pschd);
#endif
	}

#ifdef CONFIG_BBQUE_SP_YAMS_PARALLEL
	for_each(awm_thds.begin(), awm_thds.end(), mem_fn(&std::thread::join));
	awm_thds.clear();
#endif
	logger->Debug("Eval: number of entities = %d", entities.size());
}

void YamsSchedPol::EvalWorkingMode(SchedEntityPtr_t pschd) {
	std::unique_lock<std::mutex> sched_ul(sched_mtx, std::defer_lock);
	std::vector<ResID_t>::iterator ids_it;
	ResID_t bd_id;
	float sc_value    = 0.0;
	uint8_t mlog_len  = 0;
	char mlog[255];
	Timer comp_tmr;

	// Skip if the application has been disabled/stopped in the meanwhile
	if (pschd->papp->Disabled()) {
		logger->Debug("Eval: %s disabled/stopped during schedule ordering",
				pschd->papp->StrId());
		return;
	}

	// Metrics computation start
	YAMS_RESET_TIMING(comp_tmr);

	// Aggregate binding-independent scheduling contributions
	for (uint i = 0; i < YAMS_AWM_SC_COUNT; ++i) {
		GetSchedContribValue(pschd, sc_types[i], sc_value);
		pschd->metrics += sc_value;
		mlog_len += sprintf(mlog + mlog_len, "%c:%5.4f, ",
					scm->GetString(sc_types[i])[0],	sc_value);
	}
	mlog[mlog_len-2] = '\0';
	logger->Info("EvalAWM: %s metrics %s -> %5.4f",
			pschd->StrId(),	mlog, pschd->metrics);

#ifndef CONFIG_BBQUE_SP_COWS_BINDING
	// Add binding-dependent scheduling contributions
	ids_it = bindings.ids.begin();
	for (; ids_it != bindings.ids.end(); ++ids_it) {
		bd_id = *ids_it;
		if (bindings.full[bd_id]) {
			logger->Info("Eval: domain [%s%d] is full, skipping...",
					bindings.domain.c_str(), bd_id);
			continue;
		}

		// Evaluate the AWM resource binding
		SchedEntityPtr_t pschd_bd(new SchedEntity_t(*pschd.get()));
		pschd_bd->SetBindingID(bd_id);
		EvalBinding(pschd_bd, sc_value);
		pschd_bd->metrics += sc_value;

		// Insert the SchedEntity in the scheduling list
		sched_ul.lock();
		entities.push_back(pschd_bd);
		sched_ul.unlock();
		logger->Notice("Eval: %s scheduling metrics = %1.4f [%d]",
				pschd_bd->StrId(), pschd_bd->metrics, entities.size());
	}
#else
	// Insert the SchedEntity in the scheduling list
	sched_ul.lock();
	entities.push_back(pschd);
	logger->Notice("Eval: %s scheduling metrics = %1.4f [%d]",
			pschd->StrId(), pschd->metrics, entities.size());
#endif
	YAMS_GET_TIMING(coll_metrics, YAMS_METRICS_COMP_TIME, comp_tmr);
}

void YamsSchedPol::GetSchedContribValue(
		SchedEntityPtr_t pschd,
		SchedContribManager::Type_t sc_type,
		float & sc_value) {
	SchedContribManager::ExitCode_t scm_ret;
	SchedContrib::ExitCode_t sc_ret;
	EvalEntity_t const & eval_ent(*pschd.get());
	sc_value = 0.0;
	Timer comp_tmr;
	YAMS_RESET_TIMING(comp_tmr);

	// Compute the single contribution
	scm_ret = scm->GetIndex(sc_type, eval_ent, sc_value, sc_ret);
	if (scm_ret != SchedContribManager::OK) {
		logger->Error("SchedContrib: [SC manager error:%d]", scm_ret);
		if (scm_ret != SchedContribManager::SC_ERROR) {
			YAMS_RESET_TIMING(comp_tmr);
			return;
		}

		// SchedContrib specific error handling
		switch (sc_ret) {
		case SchedContrib::SC_RSRC_NO_PE:
			logger->Debug("SchedContrib: No available PEs in {%s} %d",
					bindings.domain.c_str(), pschd->bind_id);
			bindings.full.Set(pschd->bind_id);
			return;
		default:
			logger->Warn("SchedContrib: Unable to schedule in {%s} %d [err:%d]",
					bindings.domain.c_str(), pschd->bind_id, sc_ret);
			YAMS_GET_TIMING(coll_mct_metrics, sc_type, comp_tmr);
			return;
		}
	}
	YAMS_GET_TIMING(coll_mct_metrics, sc_type, comp_tmr);
	logger->Debug("SchedContrib: back from index computation");
}

YamsSchedPol::ExitCode_t YamsSchedPol::EvalBinding(
		SchedEntityPtr_t pschd_bd,
		float & value) {
	ExitCode_t result;
	float sc_value   = 0.0;
	uint8_t mlog_len = 0;
	char mlog[255];
	logger->Info("EvalBD: =========== BINDING:'%s' ID[%2d ] ===========",
			bindings.domain.c_str(), pschd_bd->bind_id);

	// Bind the resources of the AWM to the given binding domain
	result = BindResources(pschd_bd);
	if (result != YAMS_SUCCESS) {
		logger->Error("EvalBD: Resource binding failed [%d]", result);
		return result;
	}

	// Aggregate binding-dependent scheduling contributions
	value = 0.0;
	for (uint i = YAMS_AWM_SC_COUNT; i < YAMS_SC_COUNT; ++i) {
		GetSchedContribValue(pschd_bd, sc_types[i], sc_value);
		value += sc_value;
		mlog_len += sprintf(mlog + mlog_len, "%c:%5.4f, ",
					scm->GetString(sc_types[i])[0],	sc_value);
	}
	mlog[mlog_len-2] = '\0';
	logger->Info("EvalBD: %s metrics %s -> %5.4f",
			pschd_bd->StrId(), mlog, value);
	logger->Info("EvalBD: ================================================= ");

	return YAMS_SUCCESS;
}

YamsSchedPol::ExitCode_t YamsSchedPol::BindResources(SchedEntityPtr_t pschd) {
	WorkingModeStatusIF::ExitCode_t awm_result;
	AwmPtr_t & pawm(pschd->pawm);
	ResID_t & bd_id(pschd->bind_id);

	// Binding of the AWM resource into the current cluster.
	// Since the policy handles more than one binding per AWM the resource
	// binding is referenced by the ID of the bound resource
	awm_result = pawm->BindResource(bindings.type, R_ID_ANY, bd_id, bd_id);

	// The resource binding should never fail
	if (awm_result == WorkingModeStatusIF::WM_RSRC_MISS_BIND) {
		logger->Error("BindResources: AWM{%d} to resource '%s' ID=%d."
				"Incomplete	resources binding. %d / %d resources bound.",
				pawm->Id(), bindings.type, bd_id,
				pawm->GetSchedResourceBinding()->size(),
				pawm->RecipeResourceUsages().size());
		assert(awm_result == WorkingModeStatusIF::WM_SUCCESS);
		return YAMS_ERROR;
	}
	logger->Debug("BindResources: AWM{%d} to resource '%s' ID=%d",
			pawm->Id(), bindings.domain.c_str(), bd_id);

	return YAMS_SUCCESS;
}

bool YamsSchedPol::CompareEntities(SchedEntityPtr_t & se1,
		SchedEntityPtr_t & se2) {
	uint8_t gg1, gg2;

	// Metrics (primary sorting key)
	if (se1->metrics < se2->metrics)
		return false;
	if (se1->metrics > se2->metrics)
		return true;

	// Apps asserting a NAP should be considered first
	gg1 = se1->papp->GetGoalGap();
	gg2 = se2->papp->GetGoalGap();
	if ((gg1 > 0) && (gg1 >= gg2))
		return true;
	if ((gg2 > 0) && (gg2 >= gg1))
		return false;

	// Higher value AWM first
	if (se1->pawm->Value() > se2->pawm->Value())
		return true;

	return false;
}

#ifdef CONFIG_BBQUE_SP_COWS_BINDING
void YamsSchedPol::CowsSetOptBinding(SchedEntityPtr_t psch) {

	float db = atoi(std::static_pointer_cast<PluginAttr_t>\
		(psch->pawm->GetAttribute("cows", "boundness"))->str.c_str());
	float ds = atoi(std::static_pointer_cast<PluginAttr_t>\
		(psch->pawm->GetAttribute("cows", "stalls"))->str.c_str());
	float dr = atoi(std::static_pointer_cast<PluginAttr_t>\
		(psch->pawm->GetAttribute("cows", "retired"))->str.c_str());
	float df = atoi(std::static_pointer_cast<PluginAttr_t>\
		(psch->pawm->GetAttribute("cows", "flops"))->str.c_str());

	CowsApplyRecipeValues(db, ds, dr, df);
	CowsComputeBoundness(psch);
	CowsSysWideMetrics();
	CowsAggregateResults(psch);
	logger->Notice("COWS: scheduled SchedEnt on binding domain %d "
		  "with other %d apps",
		  psch->bind_id, cowsInfo.bdLoad[psch->bind_id]);
}

void YamsSchedPol::CowsUpdateMeans(SchedEntityPtr_t psch) {
	//A new application has been scheduled
	cowsInfo.bdLoad[psch->bind_id - 1]++;
	cowsInfo.bdTotalLoad++;

	//Applying the candidate SchedEnt statistics
	cowsInfo.boundnessSquaredSum[psch->bind_id - 1] +=
		pow(cowsInfo.candidatedValues[0],2);
	cowsInfo.boundnessSum[psch->bind_id - 1] +=
		cowsInfo.candidatedValues[0];
	cowsInfo.stallsSum[psch->bind_id - 1] +=
		cowsInfo.candidatedValues[1];
	cowsInfo.retiredSum[psch->bind_id - 1] +=
		cowsInfo.candidatedValues[2];
	cowsInfo.flopSum[psch->bind_id - 1] +=
		cowsInfo.candidatedValues[3];
}

void YamsSchedPol::CowsResetStatus() {

	for (int i = 0; i < bindings.num ; i++){
		cowsInfo.boundnessSquaredSum[i] = 0;
		cowsInfo.boundnessSum[i] = 0;
		cowsInfo.stallsSum[i] = 0;
		cowsInfo.retiredSum[i] = 0;
		cowsInfo.flopSum[i] = 0;
		cowsInfo.bdLoad[i] = 0;
		cowsInfo.boundnessMetrics[i] = 0;
		cowsInfo.stallsMetrics[i] = 0;
		cowsInfo.retiredMetrics[i] = 0;
		cowsInfo.migrationMetrics[i] = 0;
		cowsInfo.flopsMetrics[i] = 0;
	}
	for (int i = 0; i < 5 ; i++) cowsInfo.normStats[i] = 0;
	for (int i = 0; i < 3 ; i++) cowsInfo.modifiedSums[i] = 0;
	cowsInfo.bdTotalLoad = 0;
}

void YamsSchedPol::CowsApplyRecipeValues(
	  float db, float ds, float dr, float df) {
	// Updating system-wide means
	cowsInfo.modifiedSums[0] += ds;
	cowsInfo.modifiedSums[1] += dr;
	cowsInfo.modifiedSums[2] += df;

	// Setting info for future system update
	cowsInfo.candidatedValues[0] = db;
	cowsInfo.candidatedValues[1] = ds;
	cowsInfo.candidatedValues[2] = dr;
	cowsInfo.candidatedValues[3] = df;
}

void YamsSchedPol::CowsComputeBoundness(SchedEntityPtr_t psch) {
	float value;
	ExitCode_t result;

	logger->Info("==============| Boundness computing.. |===============");

	logger->Info("COWS: %d binding domain(s) found", bindings.num);

	// Boundness computation. Compute the delta-variance for each BD
	for (int i = 0; i < bindings.num; i++) {
		cowsInfo.boundnessMetrics[i] = 0;

		if (cowsInfo.bdLoad[i] != 0) {
			// E(X'^2) - E^2(X') - E(X^2) + E^2(X)
			cowsInfo.boundnessMetrics[i] =
				(cowsInfo.boundnessSquaredSum[i]
				+ pow(cowsInfo.candidatedValues[0], 2))
				/ (cowsInfo.bdLoad[i] + 1)
				- pow((cowsInfo.boundnessSum[i]
				+ cowsInfo.candidatedValues[0])
				/ (cowsInfo.bdLoad[i] + 1), 2)
				- (cowsInfo.boundnessSquaredSum[i])
				/ (cowsInfo.bdLoad[i])
				+ pow((cowsInfo.boundnessSum[i])
				/ (cowsInfo.bdLoad[i]), 2);

			// Only positive contributions are used to normalize
			if (cowsInfo.boundnessMetrics[i] > 0)
			  cowsInfo.normStats[0] += cowsInfo.boundnessMetrics[i];
		}
		logger->Notice("COWS: Boundness variance @BD %d for %s: %3.2f",
			      i + 1, psch->StrId(), cowsInfo.boundnessMetrics[i]);

		logger->Info("COWS: Prefetching Sys-Wide info for bd %i", i);
		cowsInfo.modifiedSums[0] += cowsInfo.stallsSum[i];
		cowsInfo.modifiedSums[1] += cowsInfo.retiredSum[i];
		cowsInfo.modifiedSums[2] += cowsInfo.flopSum[i];

		logger->Info("COWS: Prefetching Migration info for bd %i", i);

		SchedEntityPtr_t pschd_bd(new SchedEntity_t(*psch.get()));
		pschd_bd->SetBindingID(i + 1);
		result = BindResources(psch);
		if (result != YAMS_SUCCESS) {
			logger->Error("COWS: Resource binding failed [%d]", result);
		}

		// Aggregate binding-dependent scheduling contributions
		value = 0.0;
		GetSchedContribValue(pschd_bd, sc_types[SchedContribManager::MIGRATION], value);
		logger->Info("----- Migration value: %f", value);
		cowsInfo.migrationMetrics[i] = value;
		cowsInfo.normStats[4] += value;

	}
	if (cowsInfo.normStats[0] == 0) cowsInfo.normStats[0]++;
	if (cowsInfo.normStats[4] == 0) cowsInfo.normStats[4]++;
	logger->Info("Total migration denom: %f", cowsInfo.normStats[4]);
}

void YamsSchedPol::CowsSysWideMetrics() {
	// Update system mean: sumOf(BDmeans)/numberOfBDs
	cowsInfo.modifiedSums[0] /= bindings.num;
	cowsInfo.modifiedSums[1] /= bindings.num;
	cowsInfo.modifiedSums[2] /= bindings.num;

	logger->Info("===| COWS: Sys-wide variation of metrics variance |===");
	logger->Notice("COWS: Updated system wide stalls mean: %3.2f",
			cowsInfo.modifiedSums[0]);
	logger->Notice("COWS: Updated system wide retired instructions mean:"
			"%3.2f", cowsInfo.modifiedSums[1]);
	logger->Notice("COWS: Updated system wide floating point ops mean:"
			"%3.2f", cowsInfo.modifiedSums[2]);

	// For each binding domain, calculate the updated means AS IF
	// I scheduled the new app there, then calculate the corresponding
	// standard deviation
	for (int i = 0; i < bindings.num; i++) {
		logger->Info("");
		logger->Info("COWS: Computing sys metric for BD %d...", i+1);

		// Calculating standard deviations (squared). Again, if I'm on BD
		// i, the mean has changed
		for (int j = 0; j < bindings.num; j++) {
			if (j == i) {
				cowsInfo.stallsMetrics[i] +=
					pow(((cowsInfo.stallsSum[j]
					+ cowsInfo.candidatedValues[1]))
					- (cowsInfo.modifiedSums[0]),2);
				cowsInfo.retiredMetrics[i] +=
					pow(((cowsInfo.retiredSum[j]
					+ cowsInfo.candidatedValues[2]))
					- (cowsInfo.modifiedSums[1]),2);
				cowsInfo.flopsMetrics[i] +=
					pow(((cowsInfo.flopSum[j]
					+ cowsInfo.candidatedValues[3]))
					- (cowsInfo.modifiedSums[2]),2);
			}
			else if (cowsInfo.bdLoad[j] != 0) {
				cowsInfo.stallsMetrics[i] +=
					pow(((cowsInfo.stallsSum[j]))
					- (cowsInfo.modifiedSums[0]),2);
				cowsInfo.retiredMetrics[i] +=
					pow(((cowsInfo.retiredSum[j]))
					- (cowsInfo.modifiedSums[1]),2);
				cowsInfo.flopsMetrics[i] +=
					pow(((cowsInfo.flopSum[j]))
					- (cowsInfo.modifiedSums[2]),2);
			}
		}

		logger->Notice("COWS: Total stalls quadratic deviation in BD"
				"%d: %3.2f", i + 1, cowsInfo.stallsMetrics[i]);
		logger->Notice("COWS: Total ret. instructions deviation in BD"
				"%d: %3.2f", i + 1, cowsInfo.retiredMetrics[i]);
		logger->Notice("COWS: Total X87 operations deviation in BD"
				"%d: %3.2f", i + 1, cowsInfo.flopsMetrics[i]);
		logger->Info("COWS: Proceeding with next BD, if any ...");

		cowsInfo.normStats[1] += cowsInfo.stallsMetrics[i];
		cowsInfo.normStats[2] += cowsInfo.retiredMetrics[i];
		cowsInfo.normStats[3] += cowsInfo.flopsMetrics[i];
	}
}

void YamsSchedPol::CowsAggregateResults(SchedEntityPtr_t psch) {
	ExitCode_t result;
	int preferredBD = 0;
	float bestResult = 0;
	float actualResult = 0;

	logger->Info("========|  COWS: Aggregating results ... |========");

	// Normalizing
	logger->Notice(" ======================================================"
			"==================");
	for (int i = 0; i < bindings.num; i++) {
		cowsInfo.stallsMetrics[i] /= cowsInfo.normStats[1];
		cowsInfo.retiredMetrics[i] /= cowsInfo.normStats[2];
		cowsInfo.flopsMetrics[i] /= cowsInfo.normStats[3];
		cowsInfo.migrationMetrics[i] /= cowsInfo.normStats[4];

		if(cowsInfo.boundnessMetrics[i] < 0) {
			cowsInfo.boundnessMetrics[i] = 0;
		}
		else {
			cowsInfo.boundnessMetrics[i] /= cowsInfo.normStats[0];
		}

		logger->Notice("| BD %d | Bound: %3.2f | Stalls:%3.2f | "
				"Ret:%3.2f | Flops:%3.2f | Migrat:%3.2f |",
				i+1,
				cowsInfo.boundnessMetrics[i],
				cowsInfo.stallsMetrics[i],
				cowsInfo.retiredMetrics[i],
				cowsInfo.flopsMetrics[i],
				cowsInfo.migrationMetrics[i]);
	}
	logger->Notice(" ======================================================"
			"==================");

	// Calculating the best BD to schedule the app in
	bestResult = 3 * cowsInfo.boundnessMetrics[preferredBD]
			- cowsInfo.stallsMetrics[preferredBD]
			- cowsInfo.retiredMetrics[preferredBD]
			- cowsInfo.flopsMetrics[preferredBD];

	// The BDs are counted from 1, while my vectors' components are counted
	// from 0. I correcting that by setting the default preferred BD to 1
	preferredBD = 1;

	for (int i = 0; i < bindings.num; i++) {
		actualResult = 3 * cowsInfo.boundnessMetrics[i]
				- cowsInfo.stallsMetrics[i]
				- cowsInfo.retiredMetrics[i]
				- cowsInfo.flopsMetrics[i];
		if (actualResult > bestResult) {
			preferredBD = i + 1;
			bestResult  = actualResult;
		}
		logger->Info("COWS: Best result= %3.2f. BD %d result: %3.2f",
				bestResult, i + 1, actualResult);
	}

	// Set the binding ID
	psch->SetBindingID(preferredBD);
	result = BindResources(psch);
	if (result != YAMS_SUCCESS) {
		logger->Error("COWS: Resource binding failed [%d]", result);
	}

	logger->Notice("COWS: selected id is: %d.", preferredBD);
	logger->Notice("COWS: candidate values are: %3.2f, %3.2f, %3.2f, %3.2f"
			".", cowsInfo.candidatedValues[0],
			cowsInfo.candidatedValues[1],
			cowsInfo.candidatedValues[2],
			cowsInfo.candidatedValues[3]);
	logger->Info("==========|          COWS: Done             |==========");
}
#endif // CONFIG_BBQUE_SP_COWS_BINDING

} // namespace plugins

} // namespace bbque
