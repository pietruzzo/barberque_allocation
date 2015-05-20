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

#include "bbque/em/event_manager.h"
#include "bbque/utils/utility.h"

#include <fstream>
#include <sstream>

#include <boost/serialization/serialization.hpp>
//#include <boost/archive/binary_oarchive.hpp>
//#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace bbque {

std::string EventManager::TimeToString(time_t t) {
   std::stringstream strm;
   strm << t;
   return strm.str();
}

EventManager::EventManager() {
	// Get a logger module
	logger = bu::Logger::GetLogger(EVENT_MANAGER_NAMESPACE);
	assert(logger);

	std::time_t timestamp = std::time(nullptr);
	path = BBQUE_PATH_PREFIX + std::string("/var/events/events_") + TimeToString(timestamp) + std::string(".txt");

	std::ofstream ofs(path);
}

EventManager::~EventManager() {

}

EventManager & EventManager::GetInstance() {
	static EventManager instance;
	return instance;
}

void EventManager::Serialize(EventWrapper ew) {

	// create and open the archive for output
	std::ofstream ofs(path);

	if (ofs.good()) {
		try {
			boost::archive::text_oarchive oa(ofs, boost::archive::no_header);
			oa << ew;
		} catch (...) {
			logger->Error("Cannot write the archive");
		}
	} else {
		logger->Error("Cannot open archive for writing");
    }
}

EventWrapper EventManager::Deserialize() {

	EventWrapper ew;

	// open the archive for input
    std::ifstream ifs(path);

    if (ifs.good()) {
    	try {
    		boost::archive::text_iarchive ia(ifs, boost::archive::no_header);
	        ia >> ew;
	    } catch (...) {
			logger->Error("Cannot read the archive");
		}
	} else {
		logger->Error("Cannot open archive for reading");
    }

    return ew;
}

void EventManager::InitializeArchive(Event event) {
	logger->Info("Initialize Archive...");

	std::time_t timestamp = std::time(nullptr);
	event.SetTimestamp(timestamp);

	EventWrapper ew;
    ew.AddEvent(event);
    Serialize(ew);
}

void EventManager::Push(Event event) {
	logger->Info("Push Event...");

	std::time_t timestamp = std::time(nullptr);
	event.SetTimestamp(timestamp);

	EventWrapper ew;
    ew = Deserialize();
    ew.AddEvent(event);
    Serialize(ew);
}

} // namespace bbque
