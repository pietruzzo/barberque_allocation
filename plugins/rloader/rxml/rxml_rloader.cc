/*
 * Copyright (C) 2012  Politecnico di Milano
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

#include "rxml_rloader.h"

#include <algorithm>
#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>

#include "bbque/platform_proxy.h"
#include "bbque/app/application.h"
#include "bbque/app/working_mode.h"
#include "bbque/res/resource_constraints.h"
#include "bbque/utils/logging/logger.h"
#include "bbque/utils/utility.h"

namespace ba = bbque::app;
namespace br = bbque::res;
namespace bu = bbque::utils;
namespace po = boost::program_options;

namespace bbque { namespace plugins {


/** Set true it means the plugin has read its options in the config file*/
bool RXMLRecipeLoader::configured = false;

/** Recipes directory */
std::string RXMLRecipeLoader::recipe_dir = "";

/** Map of options (in the Barbeque config file) for the plugin */
po::variables_map xmlrloader_opts_value;


RXMLRecipeLoader::RXMLRecipeLoader() {
	logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	assert(logger);
	logger->Debug("Built RXML RecipeLoader object @%p", (void*)this);
}

RXMLRecipeLoader::~RXMLRecipeLoader() {
}

bool RXMLRecipeLoader::Configure(PF_ObjectParams * params) {

	if (configured)
		return true;

	// Declare the supported options
	po::options_description xmlrloader_opts_desc("RXML Recipe Loader Options");
	xmlrloader_opts_desc.add_options()
		(MODULE_CONFIG".recipe_dir", po::value<std::string>
		 (&recipe_dir)->default_value(BBQUE_PATH_PREFIX "/" BBQUE_PATH_RECIPES),
		 "recipes folder")
	;

	// Get configuration params
	PF_Service_ConfDataIn data_in;
	data_in.opts_desc = &xmlrloader_opts_desc;
	PF_Service_ConfDataOut data_out;
	data_out.opts_value = &xmlrloader_opts_value;

	PF_ServiceData sd;
	sd.id = MODULE_NAMESPACE;
	sd.request = &data_in;
	sd.response = &data_out;

	int32_t response =
		params->platform_services->InvokeService(PF_SERVICE_CONF_DATA, sd);

	if (response!=PF_SERVICE_DONE)
		return false;

	if (daemonized)
		syslog(LOG_INFO, "Using RXMLRecipeLoader recipe folder [%s]",
				recipe_dir.c_str());
	else
		fprintf(stdout, FI("Using RXMLRecipeLoader recipe folder [%s]\n"),
				recipe_dir.c_str());

	return true;
}


// =======================[ Static plugin interface ]=========================

void * RXMLRecipeLoader::Create(PF_ObjectParams *params) {
	if (!Configure(params))
		return nullptr;

	return new RXMLRecipeLoader();
}

int32_t RXMLRecipeLoader::Destroy(void *plugin) {
	if (!plugin)
		return -1;
	delete (RXMLRecipeLoader *)plugin;
	return 0;
}


// =======================[ MODULE INTERFACE ]================================

RecipeLoaderIF::ExitCode_t RXMLRecipeLoader::LoadRecipe(
		std::string const & _recipe_name,
		RecipePtr_t _recipe) {

	return RL_SUCCESS;
}

std::time_t RXMLRecipeLoader::LastModifiedTime(std::string const & _name) {
	boost::filesystem::path p(recipe_dir + "/" + _name + ".recipe");
	return boost::filesystem::last_write_time(p);
}


} // namespace plugins

} // namespace bque
