/**
 *      @file   ProcessChecker.h
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

#ifndef MPIRUN_PROCESSCHECKER_H_
#define MPIRUN_PROCESSCHECKER_H_

#include <thread>
#include <atomic>

namespace mpirun {
	class ProcessChecker {
	public:
		/**
		 * @brief Starts a thread to check for the process termination.
		 * @param pid     the pid of the process to monitor
		 * @param socket  the socket to shutdown (RW) when the process terminates
		 */
		ProcessChecker(int pid, int socket) noexcept;

		/**
		 * @brief Stops the thread that checks for temrniation.
		 */
		~ProcessChecker() noexcept;

		/**
		 * @brief Stops the thread that checks for temrniation.
		 */
		void stop() noexcept;

	private:
		std::atomic<bool> stopped;
		int pid;
		int socket;
		std::thread my_thread;

		void checker() noexcept;
	};
}
#endif /* MPIRUN_PROCESSCHECKER_H_ */
