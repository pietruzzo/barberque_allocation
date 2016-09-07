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

#ifndef BBQUE_BINDING_MANAGER_H_
#define BBQUE_BINDING_MANAGER_H_

#include "bbque/utils/logging/logger.h"


namespace bbque {

class BindingManager {

public:

	static BindingManager & GetInstance();


	virtual ~BindingManager()  {
		binding_options.clear();
	}

private:

	std::unique_ptr<bu::Logger> logger;

	BindingManager();

};

}

#endif // BBQUE_BINDING_MANAGER_H_

