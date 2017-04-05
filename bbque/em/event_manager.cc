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

#include "bbque/em/event_manager.h"
#include "bbque/utils/utility.h"

#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

using namespace std::chrono;

namespace bbque {

namespace em {

EventManager::EventManager() {
	logger = bu::Logger::GetLogger(EVENT_MANAGER_NAMESPACE);
	assert(logger);

	high_resolution_clock::time_point p = high_resolution_clock::now();
	milliseconds ms = duration_cast<milliseconds>(p.time_since_epoch());
	std::chrono::seconds s = duration_cast<seconds>(ms);
	std::time_t t  = s.count();
	std::size_t fs = ms.count() % 1000;

	std::ostringstream ostr;
	ostr << fs;
	std::string fractional_seconds = ostr.str();

	struct tm * timeinfo;
	char buffer[80];
	timeinfo = gmtime(&t);
	strftime(buffer, 80, "bbque-events_%Y_%m_%d_%T", timeinfo);
	std::string filename(buffer);

	filename     = filename + ":" + fractional_seconds + ".txt";
	archive_path = archive_folder_path + filename;

	// Create events folder if it doesn't exist
	const char* folder_path = std::string(archive_folder_path).c_str();
	boost::filesystem::path dir(folder_path);
	if(boost::filesystem::create_directory(dir)) {
		logger->Info("Create events Archive folder...");
	}

	std::ofstream ofs(archive_path);
}

EventManager::EventManager(bool external) {
	UNUSED(external);
}


EventManager & EventManager::GetInstance() {
	static EventManager instance;
	return instance;
}

void EventManager::SetArchive(std::string archive) {
	archive_path = archive_folder_path + archive;
}

void EventManager::SetPath(std::string path) {
	archive_folder_path = path;
}

void EventManager::Serialize(EventWrapper ew) {
	// Create and open the archive for output
	std::ofstream ofs(archive_path);
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
	// Open the archive for input
	std::ifstream ifs(archive_path);
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

	milliseconds timestamp = duration_cast<milliseconds>(
		system_clock::now().time_since_epoch());
	event.SetTimestamp(timestamp);

	EventWrapper ew;
	ew.AddEvent(event);
	Serialize(ew);
}

void EventManager::Push(Event event) {
	logger->Info("Push Event...");

	milliseconds timestamp = duration_cast<milliseconds>(
		system_clock::now().time_since_epoch());
	event.SetTimestamp(timestamp);

	EventWrapper ew;
	ew = Deserialize();
	ew.AddEvent(event);
	Serialize(ew);
}

} // namespace em

} // namespace bbque

