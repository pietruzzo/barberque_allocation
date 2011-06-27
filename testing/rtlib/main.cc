/**
 *       @file  main.cc
 *      @brief  A toy example application using the Barbque RTRM
 *
 * This provide a really simple (toy example) implementation for an application
 * accessing the Barbeque RTRM services.
 *
 *     @author  Patrick Bellasi (derkling), derkling@google.com
 *
 *   @internal
 *     Created  04/28/2011
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  Politecnico di Milano
 *   Copyright  Copyright (c) 2011, Patrick Bellasi
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =============================================================================
 */


#include <cstdio>
#include <iostream>
#include <random>

#include "utility.h"
#include "app.h"

#include <libgen.h>

#define FMT_DBG(fmt) BBQUE_FMT(COLOR_LGRAY,  "MAIN       [DBG]", fmt)
#define FMT_INF(fmt) BBQUE_FMT(COLOR_GREEN,  "MAIN       [INF]", fmt)
#define FMT_WRN(fmt) BBQUE_FMT(COLOR_YELLOW, "MAIN       [WRN]", fmt)
#define FMT_ERR(fmt) BBQUE_FMT(COLOR_RED,    "MAIN       [ERR]", fmt)

/**
 * The RNG used for testcase initialization.
 */
std::mt19937 rng_engine(time(0));

/**
 * The simulation timer
 */
rtrm::Timer simulation_tmr;

int main(int argc, char *argv[]) {
	uint16_t simulation_time;
	unsigned short max_reconf_time;
	unsigned short num_exc;
	unsigned short max_pt;
	unsigned short max_rt;

	std::cout << "\n\t\t.:: Simple application using the Barbque RTRM ::.\n"
		<< std::endl;

	// Dummy and dirty command line processing
	if (argc<6											||
			!sscanf(argv[1], "%hu",  &simulation_time)	||
			!sscanf(argv[2], "%hu", &max_reconf_time)	||
			!sscanf(argv[3], "%hu", &num_exc)			||
			!sscanf(argv[4], "%hu", &max_pt)			||
			!sscanf(argv[5], "%hu", &max_rt) ) {

		std::cout << "Usage: " << ::basename(argv[0]) <<
			" <st> <rt> <ne> <mp> <mr>\n"
			"Where:" << std::endl;
		std::cout << 	"<st> - simulation time [s]\n"
				"<rt> - max reconfigurations interval time [s]\n"
				"<ne> - number of EXC to register (max 999)\n"
				"<mp> - max processing time [s] for each AWM\n"
				"<mr> - max reconfiguration time [s] for each"
						"AWM switch\n"
				"\n\n" << std::endl;
		return EXIT_FAILURE;
	}

	// Upper limit the number of execution context to register
	if (num_exc>99)
		num_exc = 99;

	// Starting the simulation timer
	simulation_tmr.start();

	fprintf(stderr, FMT_INF("building application [%s]...\n"),
			::basename(argv[0]));
	BbqueApp app(::basename(argv[0]));

	char exc_name[] = "exc_000";
	fprintf(stderr, FMT_INF("registering [%03d] excution contexts...\n"),
			num_exc);
	for (uint8_t i = 0; i<num_exc; i++) {
		::snprintf(exc_name, 8, "exc_%03d", i);
		app.RegisterEXC(exc_name, i);
	}

	fprintf(stderr, FMT_INF("starting application processing...\n"));
	app.Start(0, num_exc);


	fprintf(stderr, FMT_INF("Getting an AWM for each registered EXC...\n"));
	for (uint8_t i = 0; i<num_exc; i++) {
		::snprintf(exc_name, 8, "exc_%03d", i);
		app.GetWorkingMode(exc_name);
	}

	fprintf(stderr, FMT_INF("running simulation for [%d]s\n"),
			simulation_time);

	sleep(simulation_time);

	fprintf(stderr, FMT_INF("stopping application processing...\n"));
	app.Stop(0, num_exc);


	fprintf(stderr, FMT_INF("unregistering [%03d] execution contexts...\n"),
			num_exc);
	app.UnregisterAllEXC();


	std::cout << "\n\n" << std::endl;
	return EXIT_SUCCESS;
}

