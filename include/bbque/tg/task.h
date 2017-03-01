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

#ifndef BBQUE_TG_TASK_H_
#define BBQUE_TG_TASK_H_

#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
//#include <boost/interprocess/containers/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace bbque {

class Task {

public:

	/**
	 * \brief Task constructor
	 */
	Task() {}

	/**
	 * \brief Task constructor
	 * \param _id Task id number
	 * \param _nthreads Number of threads to spawn
	 * \param _name Task name (optional)
	 */
	Task(uint32_t _id, int _nthreads = 1, std::string const & _name = "");

	/**
	 * \brief Task constructor
	 * \param _id Task id number
	 * \param _inb Input buffers id list
	 * \param _outb Output buffers id list
	 * \param _nthreads Number of threads to spawn
	 * \param _name Task name (optional)
	 */
	Task(uint32_t _id,
		std::list<uint32_t> & _inb, std::list<uint32_t> & _outb,
		int _nthreads = 1, std::string const & _name = "");


	virtual ~Task() {
		in_buffers.clear();
		out_buffers.clear();
	}

	/**
	 * \brief Task identification number
	 * \return The id number
	 */
	inline uint32_t Id() const { return id; }

	/**
	 * \brief Task name (optional)
	 * \return A string object with the task name
	 */
	inline std::string const & Name() const { return name; }


	/**
	 * \brief Number of threads spawned by this task (kernel)
	 * \note This is a useful information for the resource manager in
	 * charge of assigning a computing unit to the task
	 * \return The number of threads
	 */
	inline int GetThreadCount() const { return thread_count; }

	/**
	 * \brief Set the number of threads spawned by this task (kernel)
	 * \param nr_threads The number of threads
	 */
	inline void SetThreadCount(int nr_threads) { thread_count = nr_threads; }



	/**
	 * \brief Set the processor assigned to the task
	 * \param processor_id Identification number of the processing unit
	 */
	inline void SetMappedProcessor(int p_id) { processor_id = p_id; }

	/**
	 * \brief Get the processor assigned to the task
	 * \return The identification number of the processing unit
	 */
	inline int GetMappedProcessor() const { return processor_id; }



	/**
	 * \brief Add a buffer (id) to which read from
	 * \param buff_id Identification number of the buffer
	 */
	inline void AddInputBuffer(uint32_t buff_id) { in_buffers.push_back(buff_id); }

	/**
	 * \brief Add a buffer (id) to which write to
	 * \param buff_id Identification number of the buffer
	 */
	inline void AddOutputBuffer(uint32_t buff_id) { out_buffers.push_back(buff_id); }

	/**
	 * \brief Buffers (id) to read from
	 * \return the list of input buffers id
	 */
	inline const std::list<uint32_t> & InputBuffers() const { return in_buffers; }

	/**
	 * \brief Buffers (id) to write to
	 * \return the list of output buffers id
	 */
	inline const std::list<uint32_t> & OutputBuffers() const { return out_buffers; }

	/**
	 * \brief Event (if) for synchronization purposes
	 * \return the event identification number
	 */
	inline uint32_t Event() const { return event_id; }

	/**
	 * \brief Set the id of the event used synchronization purposes
	 * \param id the event identification number
	 */
	inline void SetEvent(uint32_t id) { event_id = id; }


private:

	uint32_t id;

	int thread_count;

	std::string name;

	int processor_id;


	std::list<uint32_t> in_buffers;

	std::list<uint32_t> out_buffers;

	// kernel function pointers

	uint32_t event_id;


	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & id;
		ar & name;
		ar & thread_count;
		ar & processor_id;
		ar & in_buffers;
		ar & out_buffers;
		ar & event_id;
	}
};

} //  namespace bbque

#endif // BBQUE_TG_TASK_H_
