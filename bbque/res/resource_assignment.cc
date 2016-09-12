/*
 * Copyright (C) 2016  Politecnico di Milano
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

#include "bbque/res/resource_assignment.h"
#include "bbque/res/resource_path.h"

namespace bbque
{
namespace res
{

ResourceAssignment::ResourceAssignment(uint64_t amount, Policy policy):
	amount(amount),
	fill_policy(policy)
{
}

ResourceAssignment::~ResourceAssignment()
{
	resources.clear();
}


void ResourceAssignment::SetResourcesList(ResourcePtrList_t & r_list)
{
	if (r_list.empty())
		return;

	resources.clear();
	mask.Reset();

	for (auto & resource: r_list) {
		resources.push_back(resource);
		mask.Set(resource->ID());
	}
}

void ResourceAssignment::SetResourcesList(
        ResourcePtrList_t & r_list,
        br::ResourceType filter_rtype,
        ResourceBitset & filter_mask)
{
	if (r_list.empty())
		return;

	resources.clear();
	mask.Reset();

	for (auto & assign_resource: r_list) {
		if ((filter_rtype == assign_resource->Type()) &&
		    (!filter_mask.Test((assign_resource->ID()))))
			continue;
		resources.push_back(assign_resource);
	}
}


void ResourceAssignment::SetResourcesList(
        ResourcePtrList_t & r_list,
        ResourceBitset const & filter_mask)
{
	if (r_list.empty())
		return;

	resources.clear();
	mask.Reset();

	for (auto & resource: r_list) {
		if (filter_mask.Test(resource->ID())) {
			resources.push_back(resource);
			mask.Set(resource->ID());
		}
	}
}

} // namespace res

} // namespace bbque
