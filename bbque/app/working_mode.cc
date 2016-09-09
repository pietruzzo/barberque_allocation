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

#include "bbque/app/working_mode.h"

#include <string>

#include "bbque/plugin_manager.h"
#include "bbque/resource_accounter.h"

#include "bbque/app/application.h"
#include "bbque/res/resource_utils.h"
#include "bbque/res/resource_path.h"
#include "bbque/res/identifier.h"
#include "bbque/res/binder.h"
#include "bbque/res/resource_assignment.h"
#include "bbque/utils/utility.h"

namespace br = bbque::res;
namespace bu = bbque::utils;

namespace bbque { namespace app {


WorkingMode::WorkingMode():
	hidden(false) {
	// Set the log string id
	strncpy(str_id, "", 12);
}

WorkingMode::WorkingMode(
		int8_t _id,
		std::string const & _name,
		float _value,
		AppSPtr_t _owner):
	id(_id), name(_name), hidden(false) {
	logger = bu::Logger::GetLogger(AWM_NAMESPACE);

	// Value must be positive
	_value > 0 ? value.recipe = _value : value.recipe = 0;

	// Set the log string id
	snprintf(str_id, 22, "{id=%02d, n=%-9s}", id, name.c_str());

	// Default value for configuration time (if no profiled)
	memset(&config_time, 0, sizeof(ConfigTimeAttribute_t));
	memset(&rt_prof, 0, sizeof(RuntimeProfiling_t));
	config_time.normal = -1;

	// Set the owner application
	if (_owner)
		SetOwner(_owner);
}

WorkingMode::~WorkingMode() {
	resources.requested.clear();
	if (resources.sync_bindings)
		resources.sync_bindings->clear();
	if (!resources.sched_bindings.empty()) {
		resources.sched_bindings.clear();
	}
}

WorkingMode::ExitCode_t WorkingMode::AddResourceRequest(
		std::string const & path_str,
		uint64_t amount,
		br::ResourceAssignment::Policy split_policy) {
	ResourceAccounter &ra(ResourceAccounter::GetInstance());
	br::ResourcePath::ExitCode_t rp_result;
	ExitCode_t result = WM_SUCCESS;

	// Init the resource path starting from the prefix
	auto resource_path = std::make_shared<br::ResourcePath>(ra.GetPrefixPath());
	if (!resource_path) {
		logger->Error("AddResourceRequest: %s '%s' invalid prefix path",
				str_id, ra.GetPrefixPath().ToString().c_str());
		return WM_RSRC_ERR_TYPE;
	}

	// Build the resource path object
	rp_result = resource_path->Concat(path_str);
	if (rp_result != br::ResourcePath::OK) {
		logger->Error("AddResourceRequest: %s '%s' invalid path",
				str_id, path_str.c_str());
		return WM_RSRC_ERR_TYPE;
	}

	// Check the existance of the resource required
	if (!ra.ExistResource(resource_path)) {
		logger->Warn("AddResourceRequest: %s '%s' not found.",
				str_id, resource_path->ToString().c_str());
		result = WM_RSRC_NOT_FOUND;
	}

	// Insert a new resource usage object in the map
	auto r_assign = std::make_shared<br::ResourceAssignment>(amount, split_policy);
	resources.requested.emplace(resource_path, r_assign);
	logger->Debug("AddResourceRequest: %s added {%s}"
			"\t[usage: %" PRIu64 "] [c=%2d]",
			str_id, resource_path->ToString().c_str(),amount,
			resources.requested.size());

	return result;
}

WorkingMode::ExitCode_t WorkingMode::Validate() {
	ResourceAccounter &ra(ResourceAccounter::GetInstance());
	uint64_t total_amount;
	hidden   = false;

	// Map of resource assignments requested
	for (auto & resource_entry: resources.requested) {
		// Current resource: path and amount required
		ResourcePathPtr_t const & path_from_recipe(resource_entry.first);
		br::ResourceAssignmentPtr_t & request_from_recipe(resource_entry.second);

		// Check the total amount available. Hide the AWM if the current total
		// amount available cannot satisfy the amount required.
		// Consider the resource template path, since the requested resource
		// can be mapped on more than one system/HW resource.
		total_amount = ra.Total(path_from_recipe, ResourceAccounter::TEMPLATE);
		if (total_amount < request_from_recipe->GetAmount()) {
			logger->Warn("%s Validate: %s usage required (%" PRIu64 ") "
				"exceeds total (%" PRIu64 ")", str_id,
				path_from_recipe->ToString().c_str(),
				request_from_recipe->GetAmount(),
				total_amount);
			hidden = true;
			logger->Warn("%s Validate: set to 'hidden'", str_id);
			return WM_RSRC_USAGE_EXCEEDS;
		}
	}

	return WM_SUCCESS;
}

uint64_t WorkingMode::RequestedAmount(ResourcePathPtr_t resource_path) const {

	for (auto & resource_entry: resources.requested) {
		ResourcePathPtr_t const & curr_path(resource_entry.first);
		if (resource_path->Compare(*(curr_path.get())) == br::ResourcePath::NOT_EQUAL)
			continue;
		return resource_entry.second->GetAmount();
	}
	return 0;
}

size_t WorkingMode::BindResource(
		br::ResourceType r_type,
		BBQUE_RID_TYPE source_id,
		BBQUE_RID_TYPE out_id,
		size_t b_refn,
		br::ResourceType filter_rtype,
		br::ResourceBitset * filter_mask) {
	br::ResourceAssignmentMap_t * source_map;
	uint32_t b_count;
	size_t n_refn;
	logger->Debug("BindResource: %s owner is %s", str_id, owner->StrId());

	// First resource binding call
	if (b_refn == 0) {
		logger->Debug("BindResource: %s binding resources from recipe", str_id);
		source_map = &resources.requested;
	}
	else {
		auto bind_it(resources.sched_bindings.find(b_refn));
		if (bind_it == resources.sched_bindings.end()) {
			logger->Error("BindResource: %s invalid binding reference [%ld]",
				str_id, b_refn);
			return 0;
		}
		source_map = (bind_it->second).get();
	}

	// Allocate a new temporary resource assignments map to store the bound
	// resource assignments
	auto out_map = std::make_shared<br::ResourceAssignmentMap_t>();
	b_count = br::ResourceBinder::Bind(
		*source_map, r_type, source_id, out_id, out_map,
		filter_rtype, filter_mask);
	if (b_count == 0) {
		logger->Warn("BindResource: %s nothing to bind", str_id);
		return 0;
	}

	// Store the resource binding
	n_refn = std::hash<std::string>()(BindingStr(r_type, source_id, out_id, b_refn));
	resources.sched_bindings[n_refn] = out_map;
	logger->Debug("BindResource: %s R{%-3s} refn[%ld] size:%d count:%d",
			str_id, br::GetResourceTypeString(r_type),
			n_refn, out_map->size(), b_count);
	return n_refn;
}

std::string WorkingMode::BindingStr(
		br::ResourceType r_type,
		BBQUE_RID_TYPE source_id,
		BBQUE_RID_TYPE out_id,
		size_t b_refn) {
	char tail_str[40];
	std::string str(br::GetResourceTypeString(r_type));
	snprintf(tail_str, 40, ",%d,%d,%ld", source_id, out_id, b_refn);
	str.append(tail_str);
	logger->Debug("BindingStr: %s", str.c_str());
	return str;
}

br::ResourceAssignmentMapPtr_t WorkingMode::GetSchedResourceBinding(size_t b_refn) const {
	auto const sched_it = resources.sched_bindings.find(b_refn);
	if (sched_it == resources.sched_bindings.end()) {
		logger->Error("GetSchedResourceBinding: %s invalid reference [%ld]",
				str_id, b_refn);
		return nullptr;
	}
	return sched_it->second;
}

WorkingMode::ExitCode_t WorkingMode::SetResourceBinding(
		br::RViewToken_t status_view,
		size_t b_refn) {
	// Set the new binding / resource assignments map
	resources.sync_bindings = GetSchedResourceBinding(b_refn);
	if (resources.sync_bindings == nullptr) {
		logger->Error("SetBinding: %s invalid scheduling binding [%ld]",
				str_id, b_refn);
		return WM_BIND_FAILED;
	}
	resources.sync_refn = b_refn;

	// Update the resource binding bit-masks
	UpdateBindingInfo(status_view, true);
	logger->Debug("SetBinding: %s resource binding [%ld] to allocate",
			str_id, b_refn);

	return WM_SUCCESS;
}


void WorkingMode::UpdateBindingInfo(
		br::RViewToken_t status_view,
		bool update_changed) {
	br::ResourceBitset new_mask;
	logger->Debug("UpdateBinding: mask update required (%s)",
			update_changed ? "Y" : "N");

	// Update the resource binding bitmask (for each type)
	for (int r_type_index = 0; r_type_index < R_TYPE_COUNT; ++r_type_index) {
		br::ResourceType r_type = static_cast<br::ResourceType>(r_type_index);
		if (r_type == br::ResourceType::PROC_ELEMENT ||
			r_type == br::ResourceType::MEMORY) {
			logger->Debug("UpdateBinding: %s R{%-3s} is terminal",
					str_id, br::GetResourceTypeString(r_type));
			// 'Deep' get bit-mask in this case
			new_mask = br::ResourceBinder::GetMask(
				resources.sched_bindings[resources.sync_refn],
				static_cast<br::ResourceType>(r_type),
				br::ResourceType::CPU,
				R_ID_ANY, owner, status_view);
		}
		else {
			new_mask = br::ResourceBinder::GetMask(
				resources.sched_bindings[resources.sync_refn],
				static_cast<br::ResourceType>(r_type));
		}
		logger->Debug("UpdateBinding: %s R{%-3s}: %s",
				str_id, br::GetResourceTypeString(r_type),
				new_mask.ToStringCG().c_str());

		// Update current/previous bitset changes only if required
		if (!update_changed || new_mask.Count() == 0) {
			logger->Debug("UpdateBinding: %s R{%-3s} mask update skipped",
				str_id, br::GetResourceTypeString(r_type));
			continue;
		}
		BindingInfo & bi(resources.binding_masks[r_type]);
		bi.SetCurrentSet(new_mask);
		logger->Debug("UpdateBinding: %s R{%-3s} changed? (%d)",
				str_id, br::GetResourceTypeString(r_type),
				bi.IsChanged());

	}
}

void WorkingMode::ClearResourceBinding() {
	resources.sync_bindings->clear();
	for (int r_type_index = 0; r_type_index < R_TYPE_COUNT; ++r_type_index) {
		br::ResourceType r_type = static_cast<br::ResourceType>(r_type_index);
		resources.binding_masks[r_type].RestorePreviousSet();
	}
}

br::ResourceBitset
WorkingMode::BindingSet(const br::ResourceType & r_type) const {
	auto const mask_it(resources.binding_masks.find(r_type));
	br::ResourceBitset empty_set;
	if (mask_it == resources.binding_masks.end())
		return empty_set;
	return mask_it->second.CurrentSet();
}

br::ResourceBitset
WorkingMode::BindingSetPrev(const br::ResourceType & r_type) const {
	auto const mask_it(resources.binding_masks.find(r_type));
	br::ResourceBitset empty_set;
	if (mask_it == resources.binding_masks.end())
		return empty_set;
	return mask_it->second.PreviousSet();
}

bool WorkingMode::BindingChanged(const br::ResourceType & r_type) const {
	auto const mask_it(resources.binding_masks.find(r_type));
	if (mask_it == resources.binding_masks.end())
		return false;
	return mask_it->second.IsChanged();
}

} // namespace app

} // namespace bbque

