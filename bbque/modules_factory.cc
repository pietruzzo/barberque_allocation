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

#include "bbque/modules_factory.h"
#include "bbque/plugins/object_adapter.h"

namespace bp = bbque::plugins;
namespace bu = bbque::utils;

namespace bbque {

/**
 * Specialize the ObjectAdapter template for Test plugins
 */
typedef bp::ObjectAdapter<bp::TestAdapter, C_Test> Test_ObjectAdapter;

plugins::TestIF * ModulesFactory::GetTestModule(const std::string & id) {
	// Build a object adapter for the TestModule
	Test_ObjectAdapter toa;

	void * module = bp::PluginManager::GetInstance().
						CreateObject(id, NULL, &toa);

	return (plugins::TestIF *) module;
}

plugins::RPCChannelIF * ModulesFactory::GetRPCChannelModule(
    std::string const & id) {
	return RPCProxy::GetInstance(id);
}

} // namespace bbque

