
# Add PowerManager main class
set (POWER_MANAGER_SRC power_manager)


# Add as library
add_library(bbque_pm STATIC ${POWER_MANAGER_SRC})

# Link static library
target_link_libraries(
	bbque_pm
	${POWER_MANAGER_LIBS}
)
