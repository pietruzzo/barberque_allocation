/*
 * Copyright (C) 2017  Politecnico di Milano
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

#ifndef BBQUE_TRIGGER_THRESHOLD_H_
#define BBQUE_TRIGGER_THRESHOLD_H_

#include "bbque/app/application.h"
#include "bbque/res/resources.h"

namespace bbque {

namespace trig {

/**
 * @class ThresholdTrigger
 * @brief Type of trigger based on threshold values
 */
class ThresholdTrigger: public Trigger {

public:

	ThresholdTrigger() {}

	virtual ~ThresholdTrigger() {}

	/**
	 * @brief The condition is verified if the current value is above the reference value
	 * for a given margin
	 * @return true in case of condition verified, false otherwise
	 */
	inline bool Check(float threshold, float curr_value, float margin = 0.0) const {
		float thres_with_margin = static_cast<float>(threshold) * (1.0 - margin);
		if (curr_value > thres_with_margin)
			return true;
		return false;
	}

};

} // namespace trig

} // namespace bbque


 #endif // BBQUE_TRIGGER_THRESHOLD_H_
