/*
 * Copyright (C) 2015  Politecnico di Milano
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

#ifndef BBQUE_MODEL_MANAGER_H_
#define BBQUE_MODEL_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "bbque/pm/models/model.h"
#include "bbque/pm/models/system_model.h"
#include "bbque/utils/logging/logger.h"


namespace bu = bbque::utils;

namespace bbque  { namespace pm {

typedef std::shared_ptr<Model> ModelPtr_t;
typedef std::map<std::string, ModelPtr_t> ModelsMap_t;
typedef std::shared_ptr<SystemModel> SystemModelPtr_t;

/**
 * @class ModelManager
 *
 * @brief Manager for the set of hardware power-thermal models
 */
class ModelManager {

public:

	/**
	 * @brief Get the (static) instance of the module
	 */
	static ModelManager & GetInstance();

	/**
	 * @brief Destructor
	 */
	virtual ~ModelManager();

	/**
	 * @brief Get a model object
	 *
	 * @param id the model string id
	 *
	 * @return A shared pointer to the model object. If the model is missing,
	 * returns the default model, coming from the Model base clas
	 * implementation.
	 */
	ModelPtr_t GetModel(std::string const & id);

	/**
	 * @brief Get the set of available models
	 *
	 * @return A referecen to the models map
	 */
	inline ModelsMap_t const & GetModels() {
		return models;
	}

	/**
	 * @brief The system power-thermal model
	 *
	 * @param model A shared pointer to the system model object
	 */
	SystemModelPtr_t GetSystemModel() {
		return system_model;
	}

	/**
	 * @brief Register a model object
	 *
	 * @param model A shared pointer to the model object
	 */
	void Register(ModelPtr_t model);

private:

	/*** Constructor */
	ModelManager();

	/*** The logger used by the model manager */
	std::unique_ptr<bu::Logger> logger;

	/*** Models map */
	ModelsMap_t models;

	/*** Default model (base class) */
	ModelPtr_t default_model;

	/***  The system power-thermal model */
	SystemModelPtr_t system_model;
};

} // namespace pm

} // namespace bbque

#endif // BBQUE_MODEL_MANAGER_H_

