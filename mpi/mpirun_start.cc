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

#include <cstdlib>
#include <string>
#include <vector>
#include <signal.h>
#include <utility>

// Network related
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "ompi_types.h"
#include "mpirun_exc.h"
#include <bbque/utils/utility.h>

#include <iostream>

namespace mpirun {

bool MpiRun::call_mpirun() {

	::setenv("BBQUE_IP",   this->bacon_external_ip.c_str(), 1);
	::setenv("BBQUE_PORT", std::to_string(bacon_external_port).c_str(), 1);

	std::string command = std::string("mpirun " );
	std::string temp;

	for (auto it = cmd_arguments.begin()+1 /* skip the program command*/;
			it != cmd_arguments.end(); it++) {
		command += std::string(*it) + " ";
	}

	// Launch mpirun in background
	// command += "&";

	logger->Notice("Forking: %s", command.c_str());
	int pid = fork();

	if (pid==0) {
		// Child!
		// Let's start mpirun and exit then

		// NOTE: call _exit and not exit() because
		// 		 exit() clear all the sockets (also the
		// 		 parent's sockets!)
		::_exit(::system(command.c_str()));
	} else {
		// Parent
		logger->Notice("Forked %i", pid);
		this->pid_child = pid;
		pc = new ProcessChecker(this->pid_child, this->socket_listening);

	}

	return true;
}

bool MpiRun::open_socket() {

	this->socket_listening = ::socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family   = AF_INET;
	serv_addr.sin_port     = htons(this->bacon_external_port);
	serv_addr.sin_addr.s_addr = bacon_listening_ip == ""
								? INADDR_ANY
								: inet_addr(this->bacon_listening_ip.c_str());

	int ret = ::bind(
			this->socket_listening,	(struct sockaddr *) &serv_addr,	sizeof(serv_addr));
	if (ret < 0) {
		logger->Fatal("Bacon unable to bind socket at %s:%i for MPI communication.",
				bacon_listening_ip.c_str(), this->bacon_external_port);
		return false;
	}

	logger->Info("Socket bound to %s:%d...",
			bacon_listening_ip.c_str(),	this->bacon_external_port);

	return ::listen(this->socket_listening, 1) == 0;	// this is one-one communication0
}


void sig_handler(int signo) {
  if (signo == SIGINT)
    printf("received SIGINT\n");
}


bool MpiRun::accept_socket() {

	::socklen_t clilen  = sizeof(this->client_addr);
	this->socket_client = ::accept(
			this->socket_listening, (struct sockaddr *) &this->client_addr,	&clilen);

	// Just to check, should not be happens. Let's stop the checker (doesn't
	// matter what happens)
	MPIRUN_SAFE_DESTROY(this->pc);
	if (this->socket_client < 0 && errno == EINVAL) {
		logger->Warn("`mpirun` terminates before opening a socket to Bacon.");
		return false;
	}

	if (this->socket_client < 0) {
		logger->Fatal("Bacon unable to accept a connection on socket "
				"for MPI communication.");
		return false;
	}
	logger->Info("`mpirun` (RAS) connected.");

	return true;
}

void MpiRun::clean_sockets() {
	// Close the sockets if opened
	if (this->socket_client != -1) {
		shutdown(this->socket_client, 2);
		this->socket_client = -1;
	}
	if (this->socket_listening != -1) {
		shutdown(this->socket_listening, 2);
		this->socket_listening = -1;
	}
}

} // End namespace
