/**
 *      @file   MpiRun_exc.h
 *      @brief  The main class for MpiRun execution.
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
	class MpiRun : public BbqueEXC {

	public:

		MpiRun(std::string const & name,
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
		RTLIB_ExitCode_t onConfigure(uint8_t awm_id);
		RTLIB_ExitCode_t onRun();
		RTLIB_ExitCode_t onMonitor();
		RTLIB_ExitCode_t onRelease();

		// Utility
		static std::vector<std::string> string_explode(std::string const &, char);


	};
}
#endif // MPIRUN_EXC_H_
