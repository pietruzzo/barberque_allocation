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

#ifndef BBQUE_BINDING_MANAGER_H_
#define BBQUE_BINDING_MANAGER_H_

#include <list>
#include <map>
#include <memory>
#include <set>

#include "bbque/resource_accounter.h"
#include "bbque/utils/logging/logger.h"


namespace bbque {

/**
 * @struct BindingInfo
 * @brief  Binding domain information
 *
 * Keep track of the runtime status of the binding domains (e.g., CPU
 * nodes)
 */
typedef struct BindingInfo {
	/// Base resource path object
	br::ResourcePathPtr_t base_path;
	/// Number of managed resource types
	std::list<br::ResourceType> r_types;
	/// Resource pointer descriptor list
	br::ResourcePtrList_t resources;
	/// The IDs of all the possible bindings
	std::set<BBQUE_RID_TYPE> ids;
	/// Keep track the bindings without available processing elements
	br::ResourceBitset full;
} BindingInfo_t;


typedef std::map<br::ResourceType, std::shared_ptr<BindingInfo_t>> BindingMap_t;


/**
 * @class BindingManager
 *
 * @brief Provides all the information to support the resource binding
 * process and a set of binding policies
 */
class BindingManager {

public:

	enum ExitCode_t {
		OK,
		ERR_MISSING_OPTIONS
	};


	static BindingManager & GetInstance();


	virtual ~BindingManager()  {
		binding_options.clear();
	}

	/**
	 * @brief Load the resource binding support information
	 *
	 * @note This can be done only when the status is READY
	 */
	ExitCode_t LoadBindingOptions();

	/**
	 * @brief The resource binding information support
	 *
	 * @return A reference to a @ref BindingMap_t object
	 */
	inline BindingMap_t & GetBindingOptions() {
		return binding_options;
	}

private:

	std::unique_ptr<bu::Logger> logger;

	ResourceAccounter & ra;

	/**
	 * A map object containing all the support information for the
	 * resource binding performed by the scheduling policy
	 */
	BindingMap_t binding_options;


	BindingManager();

	/**
	 * @brief Initialize the resource binding support information
	 */
	void InitBindingOptions();

};

}

#endif // BBQUE_BINDING_MANAGER_H_

