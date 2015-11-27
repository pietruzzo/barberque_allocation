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

#include "bbque/config.h"
#include "bbque/pm/model_manager.h"
#include "bbque/pm/models/model_arm_cortexa15.h"
#ifdef CONFIG_TARGET_ODROID_XU
#include "bbque/pm/models/system_model_odroid_xu3.h"
#endif

#define MODULE_MANAGER_NAMESPACE "bq.mm"

namespace bbque  { namespace pm {


ModelManager & ModelManager::GetInstance() {
	static ModelManager instance;
	return instance;
}

ModelManager::ModelManager() {
	logger = bu::Logger::GetLogger(MODULE_MANAGER_NAMESPACE);
	logger->Info("Model Manager initialization...");

	default_model = ModelPtr_t(new Model());
	Register(default_model);
#ifdef CONFIG_TARGET_ARM_BIG_LITTLE
	Register(ModelPtr_t(new ARM_CortexA15_Model()));
#endif

#ifdef CONFIG_TARGET_ODROID_XU
	system_model = std::make_shared<ODROID_XU3_SystemModel>();
#endif
	if (system_model == nullptr)
		logger->Warn("Model Manager: Missing a system model");
	else
		logger->Info("System model is '%s'", system_model->GetID().c_str());
}

ModelManager::~ModelManager() {
	models.clear();
}

ModelPtr_t ModelManager::GetModel(std::string const & id) {
	if (models[id] == nullptr) {
		logger->Debug("Model '%s' missing. Using default model (%s)",
				id.c_str(), default_model->GetID().c_str());
		return default_model;
	}
	return models[id];
}

void ModelManager::Register(ModelPtr_t model) {
	std::string id(model->GetID());
	models.insert(std::pair<std::string, ModelPtr_t>(id, model));
	logger->Info("Registered model '%s'", id.c_str());
}

} // namespace pm

} // namespace bbque
