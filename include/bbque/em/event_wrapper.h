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

#ifndef BBQUE_EVENT_WRAPPER_H_
#define BBQUE_EVENT_WRAPPER_H_

#include "bbque/em/event.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

namespace bbque {

class EventWrapper {

public:

	EventWrapper() {

	}

	/**
	 * @brief Constructor
	 * @param events The list of events
	 */
	EventWrapper(std::vector<Event> const & events);

	/**
	 * @brief Destructor
	 */
	~EventWrapper();

	/**
	 * @brief Get the list of events 
	 */
	inline std::vector<Event> GetEvents() {
		return this->events;
	}

	/**
	 * @brief Set the list of events
	 */
	inline void SetEvents(std::vector<Event> events) {
		this->events = events;
	}

	/**
	 * @brief Add an event to the list of events
	 */
	inline void AddEvent(Event event) {
		this->events.insert(this->events.begin(), event);
	}

private:
	
	friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	if (version == 0 || version != 0)
    	{
        	ar & events;
        }
    }
    
	std::vector<Event> events;

};

} // namespace bbque

#endif // BBQUE_EVENT_WRAPPER_H_

