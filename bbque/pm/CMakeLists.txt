# Add compilation dependencies
include_directories(
	${PROJECT_BINARY_DIR}/include/pm
	${PROJECT_SOURCE_DIR}/include/pm
)

# Add sources in the current directory to the target binary
set (POWER_MANAGER_SRC power_manager)
set (POWER_MANAGER_SRC power_manager_amd ${POWER_MANAGER_SRC})

#Add as library
add_library(bbque_pwrm STATIC ${POWER_MANAGER_SRC})

#Link static library
target_link_libraries(bbque_resources)
