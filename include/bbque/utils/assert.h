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

#ifndef BBQUE_ASSERT_H_
#define BBQUE_ASSERT_H_

#include "bbque/config.h"

#include <cassert>

#ifdef BBQUE_DEBUG
	// The following macro respect the C++ specification for assert
	#define bbque_assert(EX) (void)((EX) || (_bbque_assert (#EX, __FILE__, __LINE__),0))
#else
	#define bbque_assert(EX)
#endif

/**
 * @brief This is the custom assert logging function. You should not use
 *	  directly this function, but instead use the bbque_assert macro,
 *	  that works exactly as the std::assert one.
 */
void _bbque_assert(const char *msg, const char *file, int line);

#endif // BBQUE_ASSERT_H_
