/**
 *      @file   MpiRun_main.cc
 *      @brief  The main file of bbque-mpirun
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


#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <libgen.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "version.h"
#include "mpirun_exc.h"
#include <bbque/utils/utility.h>
#include <bbque/utils/logging/logger.h>

// Setup logging
#undef  BBQUE_LOG_MODULE
#define BBQUE_LOG_MODULE "mpirun"

namespace bu = bbque::utils;
namespace po = boost::program_options;

/**
 * @brief A pointer to an EXC
 */
std::unique_ptr<bu::Logger> logger;

/**
 * @brief A pointer to an EXC
 */
typedef std::shared_ptr<BbqueEXC> pBbqueEXC_t;

/**
 * The decription of each MpiRun parameters
 */
po::options_description opts_desc("bbque-mpirun configuration options");

/**
 * The map of all MpiRun parameters values
 */
po::variables_map opts_vm;

/**
 * The services exported by the RTLib
 */
RTLIB_Services_t *rtlib;


std::string conf_file = BBQUE_PATH_PREFIX "/" BBQUE_PATH_CONF "/bbque-mpirun.conf" ;
/**
 * @brief The recipe to use for all the EXCs
 */
std::string recipe;

/**
 * @brief Addresses per mpirun
 */
std::string config_addresses;

/**
 * @brief Slots per addresses per mpirun
 */
std::string config_slots_per_addr;

/**
 * @brief Time to wait to check new resources and reply to ompi requests
 */
int config_updatetime_resources;

/**
 * @brief The EXecution Context (EXC) registered
 */
pBbqueEXC_t pexc;

void ParseCommandLine(std::vector<const char *> &barbequeArguments) {
	// Parse command line params
	try {
	po::store(po::parse_command_line(barbequeArguments.size(), barbequeArguments.data(), opts_desc), opts_vm);
	} catch(...) {
		std::cout << "Usage: bbque-mpirun [--bbque-start optionsBBQ --bbque-end] [optionsMPI] [program]\n";
		std::cout << opts_desc << std::endl;
		::exit(EXIT_FAILURE);
	}
	po::notify(opts_vm);

	// Check for help request
	if (opts_vm.count("help")) {
		std::cout << "Usage: bbque-mpirun [--bbque-start optionsBBQ --bbque-end] [optionsMPI] [program]\n";
		std::cout << opts_desc << std::endl;
		::exit(EXIT_SUCCESS);
	}

	// Check for version request
	if (opts_vm.count("version")) {
		std::cout << "bbque-mpirun (ver. " << g_git_version << ")\n";
		std::cout << "Copyright (C) 2016 Politecnico di Milano\n";
		std::cout << "\n";
		std::cout << "Built on " <<
			__DATE__ << " " <<
			__TIME__ << "\n";
		std::cout << "\n";
		std::cout << "This is free software; see the source for "
			"copying conditions.  There is NO\n";
		std::cout << "warranty; not even for MERCHANTABILITY or "
			"FITNESS FOR A PARTICULAR PURPOSE.";
		std::cout << "\n" << std::endl;
		::exit(EXIT_SUCCESS);
	}
}

/**
 * This function separates the command line parameters, from argv to the two already allocated vectors.
 */
void splitCommandLine(
		char *argv[], int argc,
		std::vector<const char *> &outputMpiRun,
		std::vector<const char *> &outputBBQ ) {

	bool in_bbq_mode = false;

	for (int i=1; i<argc; i++) {
		if (::strcmp(argv[i],"--bbque-start") == 0) {
			in_bbq_mode = true;
			continue;
		}
		if (::strcmp(argv[i],"--bbque-end") == 0) {
			if (in_bbq_mode) {
				in_bbq_mode = false;
				continue;
			}
			else {
				std::cout << "Unconsistent use of --bbque-start and --bbque-end" << std::endl;
				::exit(EXIT_FAILURE);
			}
		}

		if (in_bbq_mode) {
			outputBBQ.push_back(argv[i]);
		}
		else {
			outputMpiRun.push_back(argv[i]);
		}
	}

	outputBBQ.insert(outputBBQ.begin(), argv[0]);
	outputMpiRun.insert(outputMpiRun.begin(), argv[0]);

}

int main(int argc, char *argv[]) {

	opts_desc.add_options()
		("help,h", "print this help message")
		("version,v", "print program version")

		("conf,c", po::value<std::string>(&conf_file)->
			default_value(conf_file),
			"bbque-MpiRun configuration file")

		("recipe,r", po::value<std::string>(&recipe)->
			default_value("MpiRun"),
			"recipe name (for all EXCs)")

		("ompi.nodes.addrs", po::value<std::string>(&config_addresses)->
				required(),
				"OpenMPI addresses")
		("ompi.nodes.slots", po::value<std::string>(&config_slots_per_addr)->
				required(),
				"OpenMPI slots per address")

		("ompi.updatetime.resources", po::value<int>
				(&config_updatetime_resources)->
				default_value(100),
				"OpenMPI time to wait to reply to ompi in milliseconds")
	;

	std::ifstream in(BBQUE_PATH_PREFIX "/" BBQUE_PATH_CONF "/" BBQUE_CONF_FILE);

	// Parse configuration file (allowing for unregistered options)
	po::store(po::parse_config_file(in, opts_desc, true), opts_vm);
	po::notify(opts_vm);

	// Setup a logger
	bu::Logger::SetConfigurationFile(conf_file);
	logger = bu::Logger::GetLogger("MpiRun");

	std::vector<const char *> mpirun_arguments;
	std::vector<const char *> barbeque_arguments;

	splitCommandLine(argv, argc, mpirun_arguments, barbeque_arguments);
	ParseCommandLine(barbeque_arguments);

	// Welcome screen
	logger->Info(".:: bbque-mpiring (ver. %s) ::.", g_git_version);
	logger->Info("Built: " __DATE__  " " __TIME__);

	// Initializing the RTLib library and setup the communication channel
	// with the Barbeque RTRM
	logger->Info("STEP 0. Initializing RTLib for mpirun [%s]...",
			::basename(argv[0]));
	RTLIB_Init(::basename(argv[0]), &rtlib);
	assert(rtlib);

	logger->Info("STEP 1. Registering mpirun using [%s] recipe...",
			recipe.c_str());
	pexc = std::make_shared<mpirun::MpiRun>(
			"MpiRun", recipe, rtlib, mpirun_arguments);
	if (!pexc->isRegistered())
		return RTLIB_ERROR;


	logger->Info("STEP 2. Starting mpirun control thread...");
	pexc->Start();

	logger->Info("STEP 3. Waiting for mpirun completion...");
	pexc->WaitCompletion();

	logger->Info("STEP 4. Disabling mpirun...");
	pexc = NULL;

	logger->Info("===== mpirun DONE! =====");
	return EXIT_SUCCESS;

}

