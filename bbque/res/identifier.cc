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

#include <cstdio>
#include <cstring>
#include <string>

#include "bbque/res/identifier.h"

namespace bbque { namespace res {


ResourceIdentifier::ResourceIdentifier(
		ResourceType _type,
		BBQUE_RID_TYPE _id):
	type(_type),
	id(_id) {

	// Sanity check
	if ((_id < R_ID_NONE) || (_id > BBQUE_MAX_R_ID_NUM)) {
		id   = R_ID_NONE;
	}

	// Set the text string form
	name.assign(GetResourceTypeString(type));
	if (id == R_ID_NONE)
		return;
	name += std::to_string(id);
}

bool ResourceIdentifier::operator< (ResourceIdentifier const & ri) {
	if (type < ri.Type())
		return true;
	if (id < ri.ID())
		return true;
	return false;
}

bool ResourceIdentifier::operator== (ResourceIdentifier const & ri) {
	if (type == ri.Type() && (id == ri.ID()))
		return true;
	return false;
}

bool ResourceIdentifier::operator!= (ResourceIdentifier const & ri) {
	if (type != ri.Type() || (id != ri.ID()))
		return true;
	return false;
}

void ResourceIdentifier::SetID(BBQUE_RID_TYPE _id) {
	id   = _id;
	name = GetResourceTypeString(type);

	// ID boundaries check
	if ((_id == R_ID_NONE) || (_id == R_ID_ANY) ||
		(_id > BBQUE_MAX_R_ID_NUM)) {
		id = R_ID_NONE;
		return;
	}
	// Update the numerical id in the name string
	name += std::to_string(id);
}

ResourceIdentifier::CResult_t ResourceIdentifier::Compare(
		ResourceIdentifier const & ri) const {
	if ((ri.type == ResourceType::UNDEFINED)
		|| (type == ResourceType::UNDEFINED))
		return DONT_CARE;
	if (ri.type != type)
		return NOT_EQUAL;
	if (ri.id != id)
		return EQUAL_TYPE;
	return EQUAL;
}


} // namespace res

} // namespace bbque

