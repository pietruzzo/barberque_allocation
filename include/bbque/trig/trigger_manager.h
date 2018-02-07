/* Copyright (C) 2017  Politecnico di Milano
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

#ifndef BBQUE_TRIGGER_MANAGER_H_
#define BBQUE_TRIGGER_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "bbque/utils/logging/logger.h"
#include "bbque/trig/trigger.h"
#include "bbque/trig/trigger_threshold.h"

#define TRIGGER_MODULE_NAME BBQUE_MODULE_NAME("trig")

namespace bbque {

namespace trig {

/**
 * @class TriggerManager
 * @brief The component responsible of collecting the instances of Trigger
 */
class TriggerManager {

public:
	static TriggerManager & GetInstance() {
		static TriggerManager instance;
		return instance;
	}

	virtual ~TriggerManager() {
		triggers.clear();
	}

	/**
	 * @brief Register a trigger
	 * @param id The identification string to retrieve it later
	 * @return The shared pointer to the trigger instance
	 */
	inline std::shared_ptr<Trigger> Register(std::string const & id) {
		if (triggers.find(id) == triggers.end())
			if (id.compare("threshold") == 0) {
				triggers[id] = std::shared_ptr<Trigger>(new ThresholdTrigger());
				logger->Debug("Added 'threshold' trigger");
			}
		return Get(id);
	}

	/**
	 * @brief Register a trigger
	 * @param id The identification string to retrieve it later
	 * @return The shared pointer to the trigger instance
	 */
	inline std::shared_ptr<Trigger> Register(std::string const & id, std::function<void()> const & trigger_func) {
		if (triggers.find(id) == triggers.end()){
			if (id.compare("threshold") == 0) {
				triggers[id] = std::shared_ptr<Trigger>(new ThresholdTrigger(trigger_func));
				logger->Debug("Added 'threshold' trigger");
			}
			
		}
		return Get(id);
	}

	/**
	 * @brief Get a trigger instance
	 * @param id The identification string to retrieve it
	 * @return The shared pointer to the trigger instance
	 */
	inline std::shared_ptr<Trigger> Get(std::string const & id) const {
		auto t_entry = triggers.find(id);
		if (t_entry != triggers.end())
			return t_entry->second;
		return nullptr;
	}

private:

	std::unique_ptr<bu::Logger> logger;

	std::map<std::string, std::shared_ptr<Trigger>> triggers;

	TriggerManager() {
		logger = bbque::utils::Logger::GetLogger(TRIGGER_MODULE_NAME);

	}

};

} // namespace trig

} // namespace bbque

#endif // BBQUE_TRIGGER_MANAGER_H_

