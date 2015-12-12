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
#define ARCHIVE_FOLDER BBQUE_PATH_PREFIX "/var/events/"

namespace bbque {

class EventManager {

public:

	/**
	 * @brief Constructor
	 */
	EventManager();

	/**
	 * @brief Constructor
	 * @param external unused parameter inserted just to differentiate
	 * the constructor from the main one (used just within barbeque)
	 */
	EventManager(bool external);
	/**
	 * @brief Destructor
	 */
	~EventManager();

	/**
	 * @brief Get the EventManager instance
	 */
	static EventManager & GetInstance();

	/**
	 * @brief Set the archive path to which the EventManager points to
	 * @param archive name of the archive
	 */
	void SetArchive(std::string archive);

	/**
	 * @brief Set the path of the folder to which the EventManager points to
	 * @param path path of the archive folder
	 */
	void SetPath(std::string path);

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
	std::string archive_path;

	/**
	 * @brief The path of the current archive folder.
	 */
	std::string archive_folder_path;

};

} // namespace bbque

#endif // BBQUE_EVENT_MANAGER_H_

