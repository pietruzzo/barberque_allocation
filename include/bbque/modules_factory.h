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

#ifndef BBQUE_MODULES_FACTORY_H_
#define BBQUE_MODULES_FACTORY_H_

#include "bbque/plugin_manager.h"
//----- Supported plugin interfaces
#include "bbque/plugins/test_adapter.h"
//#include "bbque/plugins/rpc_channel_adapter.h"
#include "bbque/plugins/platform_loader.h"
#include "bbque/plugins/recipe_loader.h"
//----- Supported C++ only plugin interfaces
#include "bbque/plugins/scheduler_policy.h"
#include "bbque/plugins/synchronization_policy.h"
#include "bbque/rpc_proxy.h"

#include <string>

namespace bbque {

/**
 * @class ModulesFactory
 * @brief A singleton class to build other BarbequeRTRM modules.
 *
 * This class provides a set of factory methods to build other Barbeque
 * modules. By design, within Barbeque each component (except core components
 * such this one)  is represented by one or more module which could be either
 * statically linked or dynamically loaded. Each such module should implement
 * a predefined interface, thus making them interchangable.
 * This class provides a set of factory methods to get a reference to each
 * kind of supported interfaces.
 */
class ModulesFactory {

public:

	template<class T> static T * GetModule(std::string const & id) {
		return (T*) plugins::PluginManager::GetInstance().CreateObject(id);
	}

	/**
	 * Get a reference to a module implementing the TestIF interface
	 */
	static plugins::TestIF * GetTestModule(
			const std::string & id = TEST_NAMESPACE);

	/**
	 * Get a reference to a module implementing the RPCChannelIF interface
	 */
	static plugins::RPCChannelIF * GetRPCChannelModule(
			std::string const & id = RPC_CHANNEL_NAMESPACE);

private:

	/**
	 * @brief   Build a new modules factory
	 */
	ModulesFactory();

};

} // namespace bbque

#endif // BBQUE_MODULES_FACTORY_H_
