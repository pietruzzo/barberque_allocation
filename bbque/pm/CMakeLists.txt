
# Add PowerManager main class
set (POWER_MANAGER_SRC power_manager)

# Add AMD GPUs power manager
if (CONFIG_BBQUE_PM_AMD)
 set (POWER_MANAGER_SRC power_manager_amd ${POWER_MANAGER_SRC})
 set (POWER_MANAGER_LIBS ${ADL_LIBRARY} ${POWER_MANAGER_LIBS})
endif (CONFIG_BBQUE_PM_AMD)

# Add CPUs power manager
if (CONFIG_BBQUE_PM_CPU)
 set (POWER_MANAGER_SRC power_manager_cpu ${POWER_MANAGER_SRC})
 set (POWER_MANAGER_LIBS cpufreq boost_regex ${POWER_MANAGER_LIBS})
endif (CONFIG_BBQUE_PM_CPU)

# Add as library
add_library(bbque_pm STATIC ${POWER_MANAGER_SRC})

# Link static library
target_link_libraries(
	bbque_pm
	${POWER_MANAGER_LIBS}
)
