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

#include "bbque/utils/logging/logger.h"
#include "bbque/utils/assert.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

#define BBQUE_ASSERT_NAMESPACE "bq.crash"

#ifdef BBQUE_DEBUG

namespace bu=bbque::utils;

void _bbque_assert(const char *msg, const char *file, int line) {

	// First of all print into the standard output. Maybe we will not able
	// to open a log, so we have to print the error into the stdout
	std::cout << "Barbeque ASSERTION FAILED." << std::endl;
	std::cout << "Assert: " << msg << std::endl;
	std::cout << "File: " << file << ":" << line << std::endl;

	// Flush the stream just to be sure before continue (and maybe crash)
	std::cout << std::flush;

	// Get a logger
	std::unique_ptr<bu::Logger> logger = 
				bu::Logger::GetLogger(BBQUE_ASSERT_NAMESPACE);

	//If we do not have a logger, we cannot log the problem, so, stdout
	// and std assert
	if (logger) {
		logger->Fatal("Barbeque ASSERTION FAILED.");
		logger->Fatal("Assert: %s", msg);
		logger->Fatal("File: %s:%d", file, line);
	}

	// Reset the logger to be sure it is flushed
	logger.reset(nullptr);

	// And then, abort
	std::abort();

	// Unreachable code
}
#endif
