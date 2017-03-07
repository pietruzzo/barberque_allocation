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


#include "tg/task.h"

namespace bbque {

Task::Task(uint32_t _id, int _nr_threads, std::string const & _name):
	id(_id), thread_count(_nr_threads), name(_name) {

}

Task::Task(uint32_t _id, std::list<uint32_t> & _inb, std::list<uint32_t> & _outb,
		int _nr_threads, std::string const & _name):
	id(_id), thread_count(_nr_threads), name(_name),
	in_buffers(_inb), out_buffers(_outb) {

}


}
