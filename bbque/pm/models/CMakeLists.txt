
# Base models
set (MODELS_SRC model system_model)

# Add here further models
#set (MODELS_SRC model_cpu...)
if (CONFIG_TARGET_ARM_BIG_LITTLE)
  set (MODELS_SRC model_arm_cortexa15 ${MODELS_SRC})
endif (CONFIG_TARGET_ARM_BIG_LITTLE)

if (CONFIG_TARGET_ODROID_XU)
	set (MODELS_SRC system_model_odroid_xu3 ${MODELS_SRC})
endif(CONFIG_TARGET_ODROID_XU)

add_library (bbque_pm_models STATIC ${MODELS_SRC})
