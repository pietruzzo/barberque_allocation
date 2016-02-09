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

#include <csignal>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <thread>
#include <chrono>

#include "process_checker.h"


namespace mpirun {

ProcessChecker::ProcessChecker(int pid, int socket) noexcept
			: pid(pid), socket(socket) {
	// Set the PID and start the thread
	this->stopped   = false;
	this->my_thread = std::thread(&ProcessChecker::checker, this);
}

void ProcessChecker::stop() noexcept {
	this->stopped = true;
}

ProcessChecker::~ProcessChecker() noexcept {
	this->stop();
	this->my_thread.join();
}

void ProcessChecker::checker() noexcept {

	while (!this->stopped) {
		pid_t result = ::waitpid(this->pid, NULL, WNOHANG);
		if (result != 0) {
			// Child terminated: close the socket so the accept() can return
			::shutdown(this->socket, SHUT_RDWR	);
			this->stopped = true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

} // namespace mpirun
