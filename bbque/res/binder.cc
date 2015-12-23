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

#include <utility>

#include "bbque/resource_accounter.h"
#include "bbque/res/binder.h"
#include "bbque/res/bitset.h"
#include "bbque/res/resource_path.h"

#define MODULE_NAMESPACE 	"bq.rbind"

namespace bu = bbque::utils;

namespace bbque { namespace res {


uint32_t ResourceBinder::Bind(
		UsagesMap_t const & src_um,
		br::ResourceIdentifier::Type_t r_type,
		br::ResID_t src_r_id,
		br::ResID_t dst_r_id,
		UsagesMapPtr_t dst_pum,
		br::ResourceIdentifier::Type_t filter_rtype,
		ResourceBitset * filter_mask) {
	ResourceAccounter &ra(ResourceAccounter::GetInstance());
	UsagesMap_t::const_iterator src_it, src_end;
	ResourcePath::ExitCode_t rp_result;
	ResourcePtrList_t r_list;
	uint32_t count = 0;
	std::unique_ptr<bu::Logger> logger = bu::Logger::GetLogger(MODULE_NAMESPACE);

	// Sanity check
	if (r_type >= br::ResourceIdentifier::TYPE_COUNT)
		return 0;

	// Proceed with the resource binding...
	src_end = src_um.end();
	for (src_it = src_um.begin(); src_it != src_um.end(); ++src_it) {
		ResourcePathPtr_t const & src_ppath(src_it->first);
		UsagePtr_t const        & src_pusage(src_it->second);

		ResourcePathPtr_t dst_ppath(new ResourcePath(src_ppath->ToString()));
		if (dst_ppath->NumLevels() == 0)
			return 0;

		// Replace ID of the given resource type with the bound ID
		rp_result = dst_ppath->ReplaceID(r_type, src_r_id, dst_r_id);
		if (rp_result == ResourcePath::OK) {
			logger->Debug("Bind: <%s> to <%s> done",
					src_ppath->ToString().c_str(),
					dst_ppath->ToString().c_str());
			++count;
		}
		else
			logger->Debug("Bind: Nothing to do in <%s>",
					src_ppath->ToString().c_str());

		// Create a new Usage object and set the binding list
		UsagePtr_t dst_pusage = std::make_shared<Usage>(
				src_pusage->GetAmount(), src_pusage->GetPolicy());
		r_list = ra.GetResources(dst_ppath);
		if ((filter_rtype != br::ResourceIdentifier::UNDEFINED)
				&& (filter_mask != nullptr))
			dst_pusage->SetResourcesList(r_list, filter_rtype, *filter_mask);
		else
			dst_pusage->SetResourcesList(r_list);

		// Insert the resource usage object in the output map
		dst_pum->insert(
				std::pair<ResourcePathPtr_t, UsagePtr_t>(dst_ppath, dst_pusage));
	}
	return count;
}

inline void SetBit(
		ResourcePathPtr_t ppath,
		br::ResourceIdentifier::Type_t r_type,
		ResourceBitset & r_mask) {
	// Get the ID of the resource type in the path
	br::ResID_t r_id = ppath->GetID(r_type);
	if ((r_id == R_ID_NONE) || (r_id == R_ID_ANY))
		return;
	// Set the ID-th bit in the mask
	r_mask.Set(r_id);
}

ResourceBitset ResourceBinder::GetMask(
		UsagesMapPtr_t pum,
		br::ResourceIdentifier::Type_t r_type) {
	return GetMask(*(pum.get()), r_type);
}

ResourceBitset ResourceBinder::GetMask(
		br::UsagesMap_t const & um,
		br::ResourceIdentifier::Type_t r_type) {
	ResourceAccounter &ra(ResourceAccounter::GetInstance());
	ResourceBitset r_mask;

	// Sanity check
	if (r_type >= br::ResourceIdentifier::TYPE_COUNT)
		return r_mask;

	// Scan the resource usages map
	for (auto & ru_entry: um) {
		br::ResourcePathPtr_t const & ppath(ru_entry.first);
		br::UsagePtr_t const & pusage(ru_entry.second);
		SetBit(ppath, r_type, r_mask);
		for (br::ResourcePtr_t const & rsrc: pusage->GetResourcesList())
			SetBit(ra.GetPath(rsrc->Path()), r_type, r_mask);
	}
	return r_mask;
}

ResourceBitset ResourceBinder::GetMask(
		UsagesMapPtr_t pum,
		br::ResourceIdentifier::Type_t r_type,
		br::ResourceIdentifier::Type_t r_scope_type,
		br::ResID_t r_scope_id,
		AppSPtr_t papp,
		RViewToken_t vtok) {
	UsagesMap_t::iterator pum_it;
	ResourceBitset r_mask;
	br::ResourceIdentifier::Type_t found_rsrc_type, found_scope_type;
	br::ResID_t found_scope_id;
	std::unique_ptr<bu::Logger> logger = bu::Logger::GetLogger(MODULE_NAMESPACE);

	// Sanity check
	if ((r_type >= br::ResourceIdentifier::TYPE_COUNT)       ||
		(r_scope_type >= br::ResourceIdentifier::TYPE_COUNT))
		return r_mask;
	logger->Debug("GetMask: scope=<%s%d> resource=<%s> view=%d",
				br::ResourceIdentifier::TypeStr[r_scope_type], r_scope_id,
				br::ResourceIdentifier::TypeStr[r_type], vtok);

	// Scan the resource usages map
	for (auto const & ru_entry: *(pum.get())) {
		br::ResourcePathPtr_t const & ppath(ru_entry.first);
		br::UsagePtr_t const & pusage(ru_entry.second);

		// From the resource path extract the "scope" type (parent type), its
		// id number, and the resource type
		found_scope_id   = ppath->GetID(r_scope_type);
		found_rsrc_type  = ppath->Type();
		found_scope_type = ppath->ParentType(found_rsrc_type);

		// Skip in case scope or resource type do not match with the required
		// values
		logger->Debug("GetMask: path=<%s>", ppath->ToString().c_str());
		if ((found_scope_type != r_scope_type) || (found_rsrc_type != r_type)) {
			logger->Debug("GetMask: skipping scope=<%s> resource=<%s>",
					br::ResourceIdentifier::TypeStr[found_scope_type],
					br::ResourceIdentifier::TypeStr[r_type]);
			continue;
		}
		logger->Debug("GetMask: search ID={%2d} found={%2d}",
				r_scope_id, found_scope_id);

		// Check the scope id number. Go deeper if it matches, or if the id
		// found is "ANY" or "NONE" => We need to inspect the list of resource
		// descriptors
		if ((found_scope_id < 0) || (found_scope_id == r_scope_id)) {
			logger->Debug("GetMask: scope <%s> found in resource <%s>!",
					br::ResourceIdentifier::TypeStr[r_scope_type],
					ppath->ToString().c_str());
			r_mask |= GetMask(pusage->GetResourcesList(),
					r_type,	r_scope_type, r_scope_id, papp, vtok);
		}
	}
	logger->Debug("GetMask: type <%s> in scope <%s> = {%s}",
			br::ResourceIdentifier::TypeStr[r_type],
			br::ResourceIdentifier::TypeStr[r_scope_type],
			r_mask.ToString().c_str());
	return r_mask;
}

ResourceBitset ResourceBinder::GetMask(
		ResourcePtrList_t const & rpl,
		br::ResourceIdentifier::Type_t r_type,
		br::ResourceIdentifier::Type_t r_scope_type,
		br::ResID_t r_scope_id,
		AppSPtr_t papp,
		RViewToken_t vtok) {
	ResourceBitset r_mask;
	std::unique_ptr<bu::Logger> logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	ResourceAccounter &ra(ResourceAccounter::GetInstance());

	// Sanity check
	if ((r_type >= br::ResourceIdentifier::TYPE_COUNT) || (!papp))
		return r_mask;

	// Scan the resources list
	for (ResourcePtr_t const & rsrc: rpl) {
		if (rsrc->Type() != r_type) {
			logger->Warn("GetMask: Skipping resource type <%s>",
					br::ResourceIdentifier::TypeStr[rsrc->Type()]);
			continue;
		}

		// Is the application using it?
		if (rsrc->ApplicationUsage(papp, vtok) == 0) {
			logger->Debug("GetMask: <%s> not used by [%s]. Skipping...",
					rsrc->Path().c_str(), papp->StrId());
			continue;
		}
		else
			logger->Debug("GetMask: <%s> used by [%s]. Continuing...",
					rsrc->Path().c_str(), papp->StrId());

		// Scope resource (type and identifier)
		br::ResourcePathPtr_t r_path(ra.GetPath(rsrc->Path()));
		if (r_path->ParentType(r_type) == r_scope_type
				&& (r_path->GetID(r_scope_type) == r_scope_id
					|| r_scope_id == R_ID_ANY)) {
			logger->Debug("GetMask: ready to set <%s>",
					br::ResourceIdentifier::TypeStr[r_type]);
			SetBit(r_path, r_type, r_mask);
		}
	}
	logger->Debug("GetMask: type <%s> = {%s}",
			br::ResourceIdentifier::TypeStr[r_type],
			r_mask.ToString().c_str());
	return r_mask;
}

ResourceBinder::ExitCode_t ResourceBinder::Compatible(
		UsagesMapPtr_t src_pum,
		UsagesMapPtr_t dst_pum) {
	UsagesMap_t::iterator src_it, src_end;
	UsagesMap_t::iterator dst_it, dst_end;

	// Different size of the maps -> not compatible
	if (src_pum->size() != dst_pum->size())
		return NOT_COMPATIBLE;

	// Compare each resource path
	for (src_it = src_pum->begin(), dst_it = dst_pum->begin();
			src_it != src_end; ++src_it, ++dst_it) {
		ResourcePathPtr_t const & src_ppath(src_it->first);
		ResourcePathPtr_t const & dst_ppath(dst_it->first);
		// The path should be equal or have equal types (template)
		if (src_ppath->Compare(*(dst_ppath.get())) == ResourcePath::NOT_EQUAL) {
			return NOT_COMPATIBLE;
		}
	}

	return OK;
}

} // namespace res

} // namespace bbque

