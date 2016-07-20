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

uint64_t ResourceAssignment::GetAmount()
{
	return amount;
}

void ResourceAssignment::SetAmount(uint64_t value)
{
	amount = value;
}

ResourcePtrList_t & ResourceAssignment::GetResourcesList()
{
	return resources;
}

void ResourceAssignment::SetResourcesList(ResourcePtrList_t & r_list)
{
	this->resources  = r_list;
	first_resource_it = r_list.begin();
	last_resource_it  = r_list.end();
}

void ResourceAssignment::SetResourcesList(
        ResourcePtrList_t & r_list,
        br::ResourceType filter_rtype,
        ResourceBitset & filter_mask)
{
	ResourcePtrListIterator_t r_it;

	if (r_list.empty())
		return;

	resources.clear();
	r_it = r_list.begin();
	for (; r_it != r_list.end(); ++r_it) {
		ResourcePtr_t rsrc(*r_it);

		// Is there a filter on the resources?
		if ((filter_rtype == rsrc->Type()) &&
		    (!filter_mask.Test((rsrc->ID()))))
			continue;

		// Copy the resource in the list and keep track of the boundary
		// iterators
		resources.push_back(rsrc);
		last_resource_it = r_it;
		if (first_resource_it == resources.end())
			first_resource_it = r_it;
	}
}


ResourcePtr_t ResourceAssignment::GetFirstResource(
        ResourcePtrListIterator_t & it)
{
	// Check if 'first_resource_it' points to a valid resource descriptor
	if (first_resource_it == resources.end())
		return ResourcePtr_t();

	// Set the argument iterator and return the shared pointer to the
	// resource descriptor
	it = first_resource_it;
	return (*it);
}

ResourcePtr_t ResourceAssignment::GetNextResource(
        ResourcePtrListIterator_t & it)
{
	do {
		// Next resource used by the application
		++it;

		// Return null if there are no more resources
		if ((it == resources.end()) || (it == last_resource_it))
			return ResourcePtr_t();
	} while ((*it)->ApplicationUsage(owner_app, status_view) == 0);

	// Return the shared pointer to the resource descriptor
	return (*it);
}

ResourceAssignment::ExitCode_t ResourceAssignment::TrackFirstResource(
        AppSPtr_t const & papp,
        ResourcePtrListIterator_t & first_it,
        RViewToken_t vtok)
{
	if (!papp)
		return RU_ERR_NULL_POINTER;

	status_view = vtok;
	owner_app   = papp;
	first_resource_it  = first_it;

	return RU_OK;
}

ResourceAssignment::ExitCode_t ResourceAssignment::TrackLastResource(
        AppSPtr_t const & papp,
        ResourcePtrListIterator_t & last_it,
        RViewToken_t vtok)
{
	if (!papp)
		return RU_ERR_NULL_POINTER;

	if (!owner_app)
		return RU_ERR_APP_MISMATCH;

	if (vtok != status_view)
		return RU_ERR_VIEW_MISMATCH;

	last_resource_it = last_it;
	return RU_OK;
}

} // namespace res

} // namespace bbque
