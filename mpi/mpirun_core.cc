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

#include "config.h"
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
std::unique_ptr<bu::Logger> mpirun_logger;

/**
 * @brief A pointer to an EXC
 */
typedef std::shared_ptr<BbqueEXC> pBbqueEXC_t;


/**
 * The services exported by the RTLib
 */
RTLIB_Services_t *rtlib;

/**
 * @brief The application configuration file
 */
std::string conf_file = BBQUE_PATH_PREFIX "/" BBQUE_PATH_CONF "/MpiRun.conf" ;

/**
 * @brief The recipe to use for all the EXCs
 */
std::string recipe;

/**
 * @brief The EXecution Context (EXC) registered
 */
pBbqueEXC_t pexc;


int main(int argc, char *argv[]) {

    mpirun::Config &config = mpirun::Config::get();

    // Setup a logger
    bu::Logger::SetConfigurationFile(conf_file);
    mpirun_logger = bu::Logger::GetLogger("MpiRun");

    config.set_arguments(argc, argv);

    // Welcome screen
    mpirun_logger->Info(".:: bbque-mpirun (ver. %s) ::.", g_git_version);
    mpirun_logger->Info("Built: " __DATE__  " " __TIME__);

    // Initializing the RTLib library and setup the communication channel
    // with the Barbeque RTRM
    mpirun_logger->Info("STEP 0. Initializing RTLib, application [%s]...",
            ::basename(argv[0]));
    RTLIB_Init(::basename(argv[0]), &rtlib);
    assert(rtlib);

    mpirun_logger->Info("STEP 1. Registering EXC using [%s] recipe...",
            recipe.c_str());
    pexc = pBbqueEXC_t(new mpirun::MpiRun("MpiRun", recipe, rtlib, config.get_mpirun_args()));
    if (!pexc->isRegistered())
        return RTLIB_ERROR;

    mpirun_logger->Info("STEP 2. Starting EXC control thread...");
    pexc->Start();


    mpirun_logger->Info("STEP 3. Waiting for EXC completion...");
    pexc->WaitCompletion();


    mpirun_logger->Info("STEP 4. Disabling EXC...");
    pexc = NULL;

    mpirun_logger->Info("===== bbque-MpiRun DONE! =====");
    return EXIT_SUCCESS;
}
