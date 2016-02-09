/**
 *      @file   ProcessChecker.cc
 *      @brief  This class checks for a process termination, then it closes the
 *              socket.
 *
 *     @author  Federico Reghenzani (federeghe), federico1.reghenzani@mail.polimi.it
 *     @author  Gianmario Pozzi (kom-broda), gianmario.pozzi@mail.polimi.it
 *
 *     Company  Politecnico di Milano
 *   Copyright  Copyright (c) 2015
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
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
	this->stopped = false;
	this->my_thread = std::thread(&ProcessChecker::checker, this);

}

void ProcessChecker::stop() noexcept {
	this->stopped = true;
}

ProcessChecker::~ProcessChecker() noexcept {
	this->stop();
	this->my_thread.join();		// Wait for thread exists
}

void ProcessChecker::checker() noexcept {

	while (!this->stopped) {

		pid_t result = ::waitpid(this->pid, NULL, WNOHANG);
		if (result != 0) {
			// The child is terminated
			::shutdown(this->socket, SHUT_RDWR	);	// Close the socket so the
													// accept blocking function can return

			this->stopped = true;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

}

}	// End namespace
