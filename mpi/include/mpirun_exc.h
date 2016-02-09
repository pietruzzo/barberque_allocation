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

#ifndef MPIRUN_EXC_H_
#define MPIRUN_EXC_H_

#include <bbque/bbque_exc.h>
#include <netinet/in.h>
#include <vector>

#include "command_manager.h"
#include "process_checker.h"

#define MPIRUN_SAFE_DESTROY(x) if (x!=NULL) { delete x; x=NULL; }


using bbque::rtlib::BbqueEXC;


namespace mpirun {

class MpiRun: public BbqueEXC {

public:

	MpiRun(
			std::string const & name,
			std::string const & recipe,
			RTLIB_Services_t *rtlib,
			std::vector<const char *> &mpirunArguments);

	~MpiRun();

private:
	const std::string bacon_external_ip      = "127.0.0.1";
	const std::string bacon_listening_ip     = "";
	const unsigned short bacon_external_port = 5858;

	std::vector<const char *> cmd_arguments;

	int pid_child        = -1;
	int socket_listening = -1;
	int socket_client    = -1;
	struct sockaddr_in serv_addr;
	struct sockaddr_in client_addr;

	res_list *all_res    = NULL;
	res_list *avail_res  = NULL;

	unsigned int avail_res_n = 0;

	ProcessChecker *pc   = NULL;
	CommandsManager *cm  = NULL;

	bool call_mpirun();
	bool open_socket();
	bool accept_socket();
	void clean_sockets();

	// Node section
	bool parse_commands();
	bool manage_nodes_requests();

	RTLIB_ExitCode_t onSetup();
	RTLIB_ExitCode_t onConfigure(int8_t awm_id);
	RTLIB_ExitCode_t onRun();
	RTLIB_ExitCode_t onMonitor();
	RTLIB_ExitCode_t onRelease();

	/**
	 * @brief Transform a string into a vector of substrings
	 *
	 * @param s the original string
	 * @param delim the delimiter
	 *
	 * @return The vector of substrings
	 */
	static std::vector<std::string> string_explode(
			std::string const & s, char	delim);

};

} // namespace mpirun

#endif // MPIRUN_EXC_H_
