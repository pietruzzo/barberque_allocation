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

#ifndef BBQUE_EVENT_H_
#define BBQUE_EVENT_H_

#include <cstdint>
#include <string>
//#include <ctime>
#include "bbque/cpp11/chrono.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/binary_object.hpp>

namespace bbque {

class Event {

public:

	Event() {

	}

	/**
	 * @brief Constructor
	 * @param validity of the event
	 * @param module the module which has triggered the event
	 * @param resource the resource destination of the event
	 * @param application the application
	 * @param type of event
	 * @param value associated to the event
	 */
	Event(bool const & valid, std::string const & module, std::string const & resource,
	 	std::string const & application, std::string const & type, 
	 	const int & value);

	/**
	 * @brief Destructor
	 */
	~Event();

	/**
     * @brief Says whether the event is valid or not
     */
    inline bool IsValid() const{
        return this->valid;
    }

    /**
     * @brief Says whether the event is valid or not
     */
    inline void SetValid(bool valid = false) {
        this->valid = valid;
    }

	/**
	 * @brief Get the module which has triggered the event
	 */
	inline std::string GetModule() const{
		return this->module;
	}

	/**
	 * @brief Get the resource destination of the event 
	 */
	inline std::string GetResource() const{
		return this->resource;
	}

	/**
	 * @brief Get the application
	 */
	inline std::string GetApplication() const{
		return this->application;
	}

	/**
	 * @brief Get the type of event
	 */
	inline std::string GetType() const{
		return this->type;
	}

	/*
	inline long GetTimestamp() const{
		return this->timestamp;
	}
	*/

	inline std::chrono::milliseconds GetTimestamp() const{
		return this->timestamp;
	}

	/**
	 * @brief Get the value associated to the event
	 */
	inline int GetValue() const{
		return this->value;
	}
	
	/*
	inline void SetTimestamp(std::chrono::milliseconds timestamp) {
		this->timestamp = (long long int)timestamp.count();
	}
	*/

	inline void SetTimestamp(std::chrono::milliseconds timestamp) {
		this->timestamp = timestamp;
	}

private:

	friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
    	if (version == 0 || version != 0)
    	{
    		ar & valid;
    		//ar & timestamp;
    		ar & boost::serialization::make_binary_object(&timestamp, sizeof(timestamp));        
	        ar & module;
	        ar & resource;
	        ar & application;
	        ar & type;
	        ar & value;
	    }
	}

	bool valid = false;
	//long long int timestamp;
    std::chrono::milliseconds timestamp;

	std::string module;

	std::string resource;

	std::string application;

	std::string type;

	int value;

};

} // namespace bbque

#endif // BBQUE_EVENT_H_

