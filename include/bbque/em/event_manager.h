/*
 * Copyright (C) 2014  Politecnico di Milano
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

#ifndef BBQUE_EVENT_MANAGER_H_
#define BBQUE_EVENT_MANAGER_H_

#include "bbque/em/event.h"
#include "bbque/em/event_wrapper.h"
#include "bbque/utils/logging/logger.h"

namespace bu = bbque::utils;

#define EVENT_MANAGER_NAMESPACE "bq.em"

namespace bbque {

class EventManager {

public:

	/**
	 * @brief Constructor
	 */
	EventManager();

	/**
	 * @brief Destructor
	 */
	~EventManager();

	/**
	 * @brief Get the EventManager instance
	 */
	static EventManager & GetInstance();

	/**
	 * @brief TimeToString
	 * @param timestamp The timestamp to convert
	 */
	std::string TimeToString(std::time_t timestamp);

	/**
	 * @brief InitializeArchive
	 * @param event The event to push
	 */
	void InitializeArchive(Event event);

	/**
	 * @brief Push
	 * @param event The event to push
	 */
	void Push(Event event);

	/**
	 * @brief Serialize
	 * @param ew The eventWrapper to serialize
	 */
	void Serialize(EventWrapper ew);

	/**
	 * @brief Deserialize
	 */
	EventWrapper Deserialize();

private:

	/**
	 * @brief The logger used by the resource manager.
	 */
	std::unique_ptr<bu::Logger> logger;

	/**
	 * @brief The path to the currently used archive.
	 */
	std::string path;

};

} // namespace bbque

#endif // BBQUE_EVENT_MANAGER_H_

