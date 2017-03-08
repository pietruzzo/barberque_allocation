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


#ifndef BBQUE_TG_POINTERS_H_
#define BBQUE_TG_POINTERS_H_

//#include <memory>
//#include "boost/interprocess/offset_ptr.hpp"
#include <boost/serialization/shared_ptr.hpp>

namespace bbque {

//#define CONFIG_STD_SHARED_PTR 1
//#ifdef CONFIG_STD_SHARED_PTR
//#define smart_pointer_type boost::interprocess::offset_ptr
// #else
//#define smart_pointer_type std::shared_ptr
#define smart_pointer_type boost::shared_ptr

//#endif
class Task;
class Buffer;
class Event;
class ArchInfo;

using TaskPtr_t   = smart_pointer_type<Task>;
using BufferPtr_t = smart_pointer_type<Buffer>;
using EventPtr_t  = smart_pointer_type<Event>;
using ArchInfoPtr_t = smart_pointer_type<ArchInfo>;

}

#endif // BBQUE_TG_POINTERS_H_
