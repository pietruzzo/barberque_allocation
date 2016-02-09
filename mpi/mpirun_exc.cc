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

#include "mpirun_exc.h"

#include <cstdio>
#include <bbque/utils/utility.h>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <utility>

#define MODULE_CONFIG "ompi"

extern std::string config_addresses;
extern std::string config_slots_per_addr;
extern int config_updatetime_resources;


namespace mpirun {

MpiRun::MpiRun(
		std::string const & name, std::string const & recipe, RTLIB_Services_t *rtlib,
		std::vector<const char *> &mpirunArguments) :
	BbqueEXC(name, recipe, rtlib), cmd_arguments(mpirunArguments) {

	logger->Info("bbque-mpirun unique identifier (UID): %u", GetUid());
}

MpiRun::~MpiRun() {
	this->clean_sockets();
	MPIRUN_SAFE_DESTROY(avail_res);
	MPIRUN_SAFE_DESTROY(all_res);
	MPIRUN_SAFE_DESTROY(this->cm);
	// This should be always == NULL at this point, but just to be sure...
	MPIRUN_SAFE_DESTROY(this->pc);
}

RTLIB_ExitCode_t MpiRun::onSetup() {
	logger->Info("MpiRun::onSetup()");
	logger->Debug("Config readed: config_addresses=%s config_slots_per_addr=%s",
			config_addresses.c_str(),
			config_slots_per_addr.c_str());

	// Load the bbq configuration
	auto ex_addresses = MpiRun::string_explode(config_addresses, ',');
	auto ex_slots     = MpiRun::string_explode(config_slots_per_addr, ',');

	// Now we have the addresses and slots, so build the pair list
	all_res = new res_list();
	for (unsigned int i=0; i<ex_addresses.size(); i++ ) {
		all_res->push_back(std::pair<std::string, int>(
				ex_addresses[i],::atoi(ex_slots[i].c_str())
				) );
	}

	if (ex_addresses.size() != ex_slots.size()) {
		logger->Crit("Configuration error: %i nodes.addrs and %i nodes.slots",
				ex_addresses.size(), ex_slots.size());
		return RTLIB_ERROR;
	}
	logger->Debug("Configuration loaded");

	if (!this->open_socket())	// Open the socket for MPI communication.
		return RTLIB_ERROR;		// if the socket fails to bind/listen/etc.
								// stops execution here.

	// Ok we can call mpirun command now (start another process, not blocking)
	logger->Debug("Ready to receive incoming connection, starting mpirun...");
	this->call_mpirun();

	// Let's accept the connection from mpirun just started
	logger->Info("Waiting for `mpirun` (RAS) incoming connection...");
	bool status = this->accept_socket();

	// Check mpirun terminates, socket problems, etc...
	if (!status)
		return RTLIB_ERROR;

	this->cm = new CommandsManager(this->socket_client);
	return RTLIB_OK;
}

RTLIB_ExitCode_t MpiRun::onConfigure(int8_t awm_id) {

	logger->Info("MpiRun::onConfigure(): AWM [%02d]", awm_id);
	// TODO: Get the bit map from BBQ

	// Avail resources is the minimum between awm_id+1 and the
	// resources list size
	avail_res_n = (size_t)awm_id+1 < all_res->size() ? (size_t)awm_id+1 : all_res->size();
	avail_res_n = all_res->size();	// Test TODO remove

	// Recreate the subvector of resource available, and update the amount of
	// available resources
	MPIRUN_SAFE_DESTROY(avail_res);
	avail_res = new res_list( all_res->begin(), all_res->begin()+avail_res_n );
	this->cm->set_available_resources(avail_res);

	return RTLIB_OK;
}

RTLIB_ExitCode_t MpiRun::onRun() {
	// Do nothing: the command has no real workload
	usleep(config_updatetime_resources*1000);	// milliseconds -> microseconds
	return RTLIB_OK;
}


RTLIB_ExitCode_t MpiRun::onMonitor() {
	RTLIB_WorkingModeParams_t const wmp = WorkingModeParams();

	logger->Info("MpiRun::onMonitor(): AWM [%02d], Cycle [%4d]",
		wmp.awm_id, Cycles());

	// Now check for MPI requests
	// If a 'true' is returned the execution should continue, 'false'
	// otherwise.
	bool status = this->cm->get_and_manage_commands();
	if (!status) {
		// Error or clean shutdown
		return RTLIB_EXC_WORKLOAD_NONE;
	}

	return RTLIB_OK;
}

RTLIB_ExitCode_t MpiRun::onRelease() {
	// TODO
	logger->Info("MpiRun::onRelease(): exit");

	return RTLIB_OK;
}


std::vector<std::string> MpiRun::string_explode(
		std::string const & s,
		char delim) {

    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim); )
        result.push_back(std::move(token));
    return result;
}

}
