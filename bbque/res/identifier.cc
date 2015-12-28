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

const char * ResourceIdentifier::TypeStr[TYPE_COUNT] = {
	"*"    ,
	"sys" ,
	"grp" ,
	"cpu" ,
	"gpu" ,
	"acc" ,
	"pe"  ,
	"mem" ,
	"icn" ,
	"io"  ,
	"cst"
};

ResourceIdentifier::ResourceIdentifier(
		ResourceIdentifier::Type_t _type,
		ResID_t _id):
	type(_type),
	id(_id) {

	// Sanity check
	if (((_id < R_ID_NONE)   || (_id > BBQUE_MAX_R_ID_NUM)) ||
		((_type < UNDEFINED) || (_type > TYPE_COUNT))) {
		type = UNDEFINED;
		id   = R_ID_NONE;
	}

	// Set the text string form
	name.assign(TypeStr[type]);
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

void ResourceIdentifier::SetID(ResID_t _id) {
	id   = _id;
	name = TypeStr[type];

	// ID boundaries check
	if ((_id == R_ID_NONE) || (_id == R_ID_ANY) ||
		(_id > BBQUE_MAX_R_ID_NUM)) {
		id = R_ID_NONE;
		return;
	}
	// Update the numerical id in the name string
	name += std::to_string(id);
}


ResourceIdentifier::Type_t ResourceIdentifier::TypeFromString(
		std::string const & _str) {
	uint8_t i = UNDEFINED;
	while (i < TYPE_COUNT) {
		if (_str.compare(ResourceIdentifier::TypeStr[i]) == 0)
			break;
		++i;
	}
	return static_cast<ResourceIdentifier::Type_t>(i);
}

const char * ResourceIdentifier::StringFromType(
		ResourceIdentifier::Type_t _type) {
	if ((_type <  UNDEFINED) ||
		(_type >= TYPE_COUNT))
		return nullptr;
	return TypeStr[_type];
}

ResourceIdentifier::CResult_t ResourceIdentifier::Compare(
		ResourceIdentifier const & ri) const {
	if ((ri.type == UNDEFINED) || (type == UNDEFINED))
		return DONT_CARE;
	if (ri.type != type)
		return NOT_EQUAL;
	if (ri.id != id)
		return EQUAL_TYPE;
	return EQUAL;
}


} // namespace res

} // namespace bbque

