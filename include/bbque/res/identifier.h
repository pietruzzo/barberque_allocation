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

#ifndef BBQUE_RESOURCE_IDENTIFIER_H_
#define BBQUE_RESOURCE_IDENTIFIER_H_

#include <bitset>
#include <memory>
#include <string>

#include "bbque/config.h"
#include "bbque/res/resource_type.h"

namespace bbque
{
namespace res
{

class ResourceIdentifier;

/** Data-type for the Resource IDs */
typedef int16_t BBQUE_RID_TYPE;
/** Shared pointer to a ResourceIdentifier object */
typedef std::shared_ptr<ResourceIdentifier> ResourceIdentifierPtr_t;

/**
 * @class ResourceIdentifier
 *
 * This is a base class providing data and function members to manage the
 * identification of a resource. More in detail, it provides access to the
 * type and the ID number of the resource.
 */
class ResourceIdentifier {

public:

	/**
	 * @enum CResult_t
	 *
	 * Results due to a comparison operation
	 */
	enum CResult_t {
		EQUAL   = 0,
		EQUAL_TYPE ,
		DONT_CARE  ,
		NOT_EQUAL
	};

	/**
	 * @brief Constructor
	 *
	 * @param type The resource type
	 * @param id The ID number of the resource
	 */
	ResourceIdentifier(ResourceType type, BBQUE_RID_TYPE id);

	/**
	 * @brief Operator < overloading
	 *
	 * @return true if type < type of comparing resource identifier.
	 * if the type is the same, compare the ID numbers.
	 */
	bool operator< (ResourceIdentifier const &);

	/**
	 * @brief Resource identity as a text string
	 *
	 * @return A string made by type + ID
	 */
	inline std::string const & Name() const {
		return name;
	}

	/**
	 * @brief Get the resource ID number
	 *
	 * @return The ID number
	 */
	inline BBQUE_RID_TYPE ID() const {
		return id;
	}

	/**
	 * @brief Set the resource ID number
	 *
	 * @param _id The ID number to set
	 */
	void SetID(BBQUE_RID_TYPE _id);

	/**
	 * @brief Get the resource type
	 *
	 * @return The resource type
	 */
	inline ResourceType Type() const {
		return type;
	}

	/**
	 * @brief Set the resource type
	 */
	inline void SetType(ResourceType _type) {
		type = _type;
	}

	/**
	 * @brief Compare two resource identifiers
	 *
	 * @param ri The resource identifier to compare
	 * @return A CResult_t value.
	 * <li>
	 * 	<ul>EQUAL if type and ID are the same
	 * 	<ul>EQUAL_TYPE for equal type but different ID
	 * 	<ul>NOT_EQUAL when both ID and type are different
	 * </li>
	 */
	CResult_t Compare(ResourceIdentifier const & ri) const;

protected:

	/** Resource name (i.e. "mem0", "pe1", "dma1", ...) */
	std::string name;

	/** Type of resource */
	ResourceType type;

	/** ID of the resource */
	BBQUE_RID_TYPE id;

};


}

}

#endif // BBQUE_RESOURCE_IDENTIFIER_H_

