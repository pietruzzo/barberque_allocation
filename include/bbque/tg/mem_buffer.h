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

#ifndef BBQUE_TG_BUFFER_H_
#define BBQUE_TG_BUFFER_H_

#include <boost/serialization/list.hpp>
//#include <boost/interprocess/containers/list.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace bbque {

class Buffer {

public:

	Buffer() {}

	Buffer(uint32_t _id, size_t _size, uint64_t _addr = 0);

	virtual  ~Buffer() {
		writer_tasks.clear();
		reader_tasks.clear();
	}

	inline uint32_t Id() const { return id; }

	inline size_t Size() const { return size_in_bytes; }

	inline uint32_t PhysicalAddress() const { return phy_addr; }

	inline void SetPhysicalAddress(uint64_t address) {
		phy_addr = address;
	}


	inline const std::list<uint32_t> & WriterTasks() const {
		return writer_tasks;
	}

	inline const std::list<uint32_t> & ReaderTasks() const {
		return reader_tasks;
	}


	inline void AddWriterTask(uint32_t task_id) {
		writer_tasks.push_back(task_id);
	}

	inline void AddReaderTask(uint32_t task_id) {
		reader_tasks.push_back(task_id);
	}

	inline void RemoveWriterTask(uint32_t task_id) {
		writer_tasks.remove(task_id);
	}

	inline void RemoveReaderTask(uint32_t task_id) {
		reader_tasks.remove(task_id);
	}

	inline void RemoveTask(uint32_t task_id) {
		RemoveWriterTask(task_id);
		RemoveReaderTask(task_id);
	}


	inline uint32_t Event() const { return event_id; }



private:
	uint32_t id;

	uint64_t phy_addr;

	size_t size_in_bytes;


	std::list<uint32_t> writer_tasks;

	std::list<uint32_t> reader_tasks;


	uint32_t event_id;


	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & id;
		ar & phy_addr;
		ar & size_in_bytes;
		ar & writer_tasks;
		ar & reader_tasks;
		ar & event_id;
	}
};

} // bbque

#endif // ifndef BBQUE_TG_BUFFER_H
