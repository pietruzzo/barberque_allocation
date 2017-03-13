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

#ifndef BBQUE_TG_HW_H
#define BBQUE_TG_HW_H

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace bbque {


enum class ArchType {
	NONE,
	GN,
	ARM,
	PEAK,
	NUP,

	STOP
};


class ArchInfo {

public:

	ArchInfo():
		priority(0), address(0), binary_size(0), stack_size(0) {
	}

	ArchInfo(uint8_t _prio, size_t _addr, size_t _bins, size_t _ss):
		priority(_prio), address(_addr),
		binary_size(_bins), stack_size(_ss) {
	}


	/**
	 * \brief Set the priority of HW target architecture supported
	 * \param priority If the application specified a preference for
	 * each target. priority=0 means highest preference value.
	 */
	inline void SetPriority(uint8_t _prio) { priority = _prio; }

	/**
	 * \brief Get the priority of HW target architecture supported
	 * \param priority If the application specified a preference for
	 * each target. priority=0 means highest preference value.
	 */
	inline uint8_t Priority() const { return priority; }


	/**
	 * \brief The memory address to which deploy the task binary
	 * \return The memory address
	 */
	inline size_t Address() const { return address; }

	/**
	 * \brief Set the memory address to which deploy the task binary
	 * \param _addr The memory address to set
	 */
	inline void SetAddress(size_t _addr) { address = _addr; }


	/**
	 * \brief The current task binary size
	 * \return The memory address
	 */
	inline size_t BinarySize() const { return binary_size; }

	/**
	 * \brief Set the current task binary size
	 * \return The memory address
	 */
	inline void SetBinarySize(size_t _binsize) { binary_size = _binsize; }


	/**
	 * \brief The stack size for the task execution
	 * \return The memory stack size
	 */
	inline size_t StackSize() { return stack_size; }

	/**
	 * \brief Set the stack size for the task executio
	 * \param _ss The memory stack size to set
	 */
	inline void SetStackSize(size_t _ss) { stack_size = _ss; }


protected:

	uint8_t priority;

	size_t address;

	size_t binary_size;

	size_t stack_size;


	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		(void) version;
		ar & priority;
		ar & address;
		ar & binary_size;
		ar & stack_size;
	}
};


}


#endif // BBQUE_TG_HW_H
