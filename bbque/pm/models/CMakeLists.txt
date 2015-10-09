
# Base model
set (MODELS_SRC model)

# Add here further models
#set (MODELS_SRC model_cpu...)
if (CONFIG_TARGET_ARM_BIG_LITTLE)
  set (MODELS_SRC model_arm_cortexa15 ${MODELS_SRC})
endif (CONFIG_TARGET_ARM_BIG_LITTLE)

add_library (bbque_pm_models STATIC ${MODELS_SRC})
