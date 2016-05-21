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

#ifndef MPIRUN_COMMANDSMANAGER_H_
#define MPIRUN_COMMANDSMANAGER_H_

#include <string>
#include <vector>
#include <bbque/utils/logging/logger.h>

extern std::unique_ptr<bbque::utils::Logger> mpirun_logger;


namespace mpirun {

typedef std::vector<std::pair<std::string, int>> res_list;

class CommandsManager {

public:

	/**
	 * @brief The constructor. It just initializes the object and save the
	 *        socket passed as argument.
	 * @param socket the socket of RAS client connected
     * @param avail_res the list of available resources from bbque
	 */
    CommandsManager(int socket, std::shared_ptr<res_list> avail_res) noexcept;


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
	inline bool get_if_error() const noexcept {
		return this->error;
	}

    /**
     * @brief Request to OpenMPI a migration of a daemon
     * @param The source node with the same node used in allocation
     * @param The destinatio node with the same node used in allocation
     */
    void request_migration(const std::string &src, const std::string &dst);

	/**
	 * @brief Setter for the list of available resources
	 *
	 * You must call this method with valid resources before any call
	 * to get_and_manage_commands().
	 */
    inline void set_available_resources(std::shared_ptr<const res_list> av) noexcept {
		this->available_resources = av;
	}

	/**
	 * @brief Getter for previous set list of available_resources
	 *
	 * @return The list of available resources. If resources are never been
	 * set NULL is returned.
	 */
    inline std::shared_ptr<const res_list> get_available_resources() const noexcept {
		return this->available_resources;
	}

private:

	bool error = false;
    bool mig_is_available = false;

	int socket_client;

    std::shared_ptr<const res_list> available_resources;

	bool manage_nodes_request() noexcept;

};

} // namespace mpirun

#endif // MPIRUN_COMMANDSMANAGER_H_
