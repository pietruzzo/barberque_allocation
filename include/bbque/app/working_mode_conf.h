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

#ifndef BBQUE_WORKING_MODE_CONF_IF_H
#define BBQUE_WORKING_MODE_CONF_IF_H

#include "bbque/app/working_mode_status.h"

namespace br = bbque::res;

namespace bbque { namespace app {

// Forward declaration
class WorkingModeConfIF;

/** Shared pointer to the class here defined */
typedef std::shared_ptr<WorkingModeConfIF> AwmCPtr_t;

/**
 * @brief Working Mode configureation interfaace
 *
 * This is the working mode interface used for runtime information updating
 */
class WorkingModeConfIF: public WorkingModeStatusIF {

public:

	/**
	 * @brief Bind resource assignments to system resources descriptors
	 *
	 * Resource paths taken from the recipes use IDs that do not care about
	 * the real system resource IDs registered by BarbequeRTRM. Thus a binding
	 * must be solved in order to make the request of resources satisfiable.
	 *
	 * The function member takes the resource type we want to bind (i.e.
	 * "CPU"), its source ID number (as specified in the recipe), and the
	 * destination ID related to the system resource to bind. Thus it builds
	 * a UsagesMap, wherein the ResourcePath objects, featuring the resource
	 * type specified, have the destination ID number in place of the source
	 * ID.
	 *
	 * @param r_type The type of resource to bind
	 * @param src_ID Recipe resource name ID
	 * @param dst_ID System resource name ID
	 * @param b_refn [optional] Reference number of an already started binding
	 * @param filter_rtype [optional] Second level resource type
	 * @param filter_mask  [optional] Binding mask for the second level
	 * resource type
	 *
	 * @return The number of resource binding performed
	 * @note Use R_ID_ANY if you want to bind the resource without care
	 * about its ID.
	 */
	virtual size_t BindResource(
			br::ResourceType r_type,
			BBQUE_RID_TYPE src_ID,
			BBQUE_RID_TYPE dst_ID,
			size_t b_refn = 0,
			br::ResourceType filter_rtype =
				br::ResourceType::UNDEFINED,
			br::ResourceBitset * filter_mask = nullptr) = 0;

	/**
	 * @brief Clear the resource binding to schedule
	 *
	 * The method cancel the current resource binding "to schedule", i.e. the
	 * map of resource assignments before builds through BindResource(), and not
	 * already set.
	 */
	void ClearSchedResourceBinding();
};

} // namespace app

} // namespace bbque

#endif // BBQUE_WORKING_MODE_CONF_IF_H
