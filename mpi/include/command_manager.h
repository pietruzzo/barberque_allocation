/**
 *      @file  CommandsManager.h
 *      @brief  The core class to manage command incoming from `mpirun` RAS.
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

#ifndef MPIRUN_COMMANDSMANAGER_H_
#define MPIRUN_COMMANDSMANAGER_H_

#include <string>
#include <vector>
#include <bbque/utils/logging/logger.h>

namespace mpirun {

typedef std::vector<std::pair<std::string, int>> res_list;

class CommandsManager {

public:

	/**
	 * @brief The constructor. It just initializes the object and save the
	 *        socket passed as argument.
	 * @param socket    the socket of ras client connected
	 */
	CommandsManager(int socket) noexcept;


	/**
	 * @brief The core method. It checks if new messages are on socket and in
	 *        in case it manages them. If there is no new messages, it simply
	 *        return successfully. (Note that this call is non-blocking)
	 *
	 * @note  You MUST call set_available_resources() before any call to this
	 *        method!
	 *
	 * @return it returns false if wants to terminate
	 * 	       the execution. Termination due to error or due to
	 *         normal closing is defined by get_in_error method.
	 *         It returns true if messages are elaborated successfully
	 *         or no new messages are present.
	 */
	bool get_and_manage_commands() noexcept;

	/**
	 * @brief It returns true if an error occurs, false otherwise.
	 */
	bool get_if_error()      const noexcept { return this->error; }

	/**
	 * @brief Setter for the list of available resources. You must
	 *        call this method with valid resources before any call
	 *        to get_and_manage_commands().
	 */
	void set_available_resources(const res_list *av) noexcept {this->available_resources = av;}

	/**
	 * @brief getter for previous setted list of available_resources.
	 * @return the list of available resources. If resources are never been
	 *         setted NULL is returned.
	 */
	const res_list *get_available_resources() const noexcept {return this->available_resources;}

private:
	bool error = false;
	int socket_client;

	const res_list *available_resources = NULL;
	static std::unique_ptr<bbque::utils::Logger> logger;

	bool manage_nodes_request() noexcept;

};

}

#endif /* INCLUDE_COMMANDSMANAGER_H_ */
