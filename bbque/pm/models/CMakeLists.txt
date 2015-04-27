
# Base model
set (MODELS_SRC model)

# Add here further models
#set (MODELS_SRC model_cpu...)

add_library (bbque_pm_models STATIC ${MODELS_SRC})
