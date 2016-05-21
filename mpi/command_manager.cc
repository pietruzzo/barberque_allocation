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

#include "command_manager.h"
#include "ompi_types.h"

// STD
#include <cstring>
#include <iostream>

// Network
#include <sys/socket.h>
#include <fcntl.h>

// BBQ
#include <bbque/utils/utility.h>

namespace mpirun {

CommandsManager::CommandsManager(int socket, std::shared_ptr<res_list> avail_res) noexcept
        : socket_client(socket), available_resources(avail_res) {
    // Constructor
}


void CommandsManager::request_migration(const std::string &src, const std::string &dst) {

    if ( ! this->mig_is_available ) {
        mpirun_logger->Error("Migration is not available.");
        return;
    }

    local_bbq_cmd_t cmd;
    cmd.jobid = 0;
    cmd.cmd_type = BBQ_CMD_MIGRATE;
    cmd.flags = 0;
    send(this->socket_client, &cmd, sizeof(local_bbq_cmd_t), 0);

    local_bbq_migrate_t mig_cmd;
    mig_cmd.jobid = 0;
    strncpy(mig_cmd.src, src.c_str(), 255);
    strncpy(mig_cmd.dest, dst.c_str(), 255);
    send(this->socket_client, &mig_cmd, sizeof(local_bbq_migrate_t), 0);

}


bool CommandsManager::get_and_manage_commands() noexcept {
	local_bbq_cmd_t cmd;

	// Now set the socket as non-blocking
	::fcntl(this->socket_client,
			F_SETFL, ::fcntl(this->socket_client,F_GETFL, 0) | O_NONBLOCK);

	// First of all, receive a command from the shell
	int bytes = recv(this->socket_client, &cmd, sizeof(local_bbq_cmd_t), 0);
	if (bytes==-1 && errno == EWOULDBLOCK) {
		// No requests (remember, recv is non-blocking!)
		return true;
	}

	if(bytes != sizeof(local_bbq_cmd_t)) {
		this->error = true;
        mpirun_logger->Crit("Error receiving data from `mpirun` (maybe dirty close?)");
		return false;
    }

	// Now reset the socket as blocking
	::fcntl(this->socket_client,
			F_SETFL, ::fcntl(this->socket_client,F_GETFL, 0) & ~O_NONBLOCK);

	switch(cmd.cmd_type) {
    case BBQ_CMD_NODES_REQUEST:
        if ( cmd.flags & BBQ_OPT_MIG_AVAILABLE ) {
            mpirun_logger->Info("`mpirun` reports migration capability.");
            this->mig_is_available = true;
        }
        return this->manage_nodes_request();
    break;
    case BBQ_CMD_TERMINATE:
        mpirun_logger->Info("`mpirun` is shutting down, closing...");
        return false;
    break;

    case BBQ_CMD_MIGRATION_READY:
        mpirun_logger->Info("`mpirun` reports all nodes are ready for migration.");
        return true;
    break;

    case BBQ_CMD_MIGRATION_ONGOING:
        mpirun_logger->Info("`mpirun` reports migration ongoing.");
        return true;
    break;

    case BBQ_CMD_MIGRATION_ABORTED:
        mpirun_logger->Crit("Migration aborted.");
        return true;
    break;

    case BBQ_CMD_MIGRATION_SUCCEEDED:
        mpirun_logger->Info("Migration succeeded.");
        return true;
    break;

    default:
        mpirun_logger->Error("Received unknown command, ignoring...");
        return true;    // not-strict mode: ignore unknown command
    break;

	}
}

bool CommandsManager::manage_nodes_request() noexcept {
	local_bbq_job_t job;

	int bytes  = recv(this->socket_client, &job, sizeof(local_bbq_job_t), 0);
	if (bytes != sizeof(local_bbq_job_t)) {
		// Something bad happened here
        mpirun_logger->Crit("Error receiving data from `mpirun`");
		return false;
	}

    mpirun_logger->Notice((std::string("Requests #" + std::to_string(job.slots_requested)
					+ " nodes for ") + std::to_string(job.jobid)).c_str());

	// First of all, send the command (preamble) to the client
	local_bbq_cmd_t cmd_to_send;
	cmd_to_send.jobid    = job.jobid;
	cmd_to_send.cmd_type = BBQ_CMD_NODES_REPLY;

	bytes = send(this->socket_client, &cmd_to_send, sizeof(local_bbq_cmd_t), 0);
	if (bytes != sizeof(local_bbq_cmd_t)) {
		// Something bad happened here
        mpirun_logger->Crit("Error sending cmd reply to `mpirun`");
		return false;
	}

	// Now we can send all resources available.
	int size = available_resources->size();
	for (int i=0; i < size; i++) {
		local_bbq_res_item_t to_send;
		to_send.jobid = job.jobid;
		::strncpy(to_send.hostname, (*available_resources)[i].first.c_str(), 256);
		to_send.slots_available = (*available_resources)[i].second;
		to_send.more_items      = (i >= size-1) ? 0 : 1;

        mpirun_logger->Info((std::string("Sending node, more items: ")
					+ std::to_string(to_send.more_items)).c_str());

		bytes = send(this->socket_client, &to_send, sizeof(local_bbq_res_item_t), 0);
		if (bytes != sizeof(local_bbq_res_item_t)) {
            mpirun_logger->Crit("Error sending nodes reply to `mpirun`");
			return false;
		}
	}

	return true;
}

}	// End of namespace
