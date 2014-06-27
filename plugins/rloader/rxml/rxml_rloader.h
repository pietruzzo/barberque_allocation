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

#ifndef BBQUE_RXML_RLOADER_H_
#define BBQUE_RXML_RLOADER_H_

#include "bbque/plugins/recipe_loader.h"

#include "bbque/modules_factory.h"
#include "bbque/plugin_manager.h"
#include "bbque/plugins/plugin.h"
#include "bbque/utils/logging/logger.h"
#include "bbque/utils/attributes_container.h"

#define MODULE_NAMESPACE RECIPE_LOADER_NAMESPACE".rxml"
#define MODULE_CONFIG RECIPE_LOADER_CONFIG".rxml"

namespace bu = bbque::utils;

using bbque::app::Application;
using bbque::app::AppPtr_t;
using bbque::app::AwmPtr_t;
using bbque::app::WorkingMode;
using bbque::utils::AttributesContainer;

// Parameters received by the PluginManager on create calls
struct PF_ObjectParams;

namespace bbque { namespace plugins {

// Return code for internal purpose
#define __RSRC_SUCCESS 		0x0
#define __RSRC_WEAK_LOAD 	0x1
#define __RSRC_FORMAT_ERR 	0x2

// Max string length for the plugins data
#define PDATA_MAX_LEN 	20

/**
 * @brief RXML recipe loader plugin
 *
 */
class RXMLRecipeLoader: public RecipeLoaderIF {

public:

	/**
	 * @brief Method for creating the static plugin
	 */
	static void * Create(PF_ObjectParams *);

	/**
	 * @brief Method for destroying the static plugin
	 */
	static int32_t Destroy(void *);

	/**
	 * Default destructor
	 */
	virtual ~RXMLRecipeLoader();

	/**
	 * @see RecipeLoaderIF
	 */
	ExitCode_t LoadRecipe(std::string const & recipe_name, RecipePtr_t recipe);

	/**
	 * @see RecipeLoaderIF
	 */
	std::time_t LastModifiedTime(std::string const & recipe_name);

private:

	/**
	 * @brief System logger instance
	 */
	std::unique_ptr<bu::Logger> logger;

	/**
	 * Set true when the recipe loader has been configured.
	 * This is done by parsing a configuration file the first time a
	 * recipe loader is created.
	 */
	static bool configured;

	/**
	 * The directory path containing all the application recipes.
	 * Each recipe is an XML file named with suffix <tt>.recipe</tt>
	 */
	static std::string recipe_dir;

	/**
	 * Shared pointer to the recipe object
	 */
	RecipePtr_t recipe_ptr;

	/**
	 * The constructor
	 */
	RXMLRecipeLoader();

	/**
	 * @brief Load RecipeLoader configuration
	 *
	 * @param params @see PF_ObjectParams
	 * @return True if the configuration has been properly loaded and object
	 * could be built, false otherwise
	 */
	static bool Configure(PF_ObjectParams * params);

};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_RXML_RLOADER_H_
