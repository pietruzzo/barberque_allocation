
#----- Add "GRIDBALANCE" target dynamic library

if (NOT CONFIG_BBQUE_SCHEDPOL_GRIDBALANCE)
	return(gridbalance)
endif(NOT CONFIG_BBQUE_SCHEDPOL_GRIDBALANCE)

# Set the macro for the scheduling policy loading
if (CONFIG_BBQUE_SCHEDPOL_DEFAULT_GRIDBALANCE)
  set (BBQUE_SCHEDPOL_DEFAULT "gridbalance" CACHE STRING
	  "Setting scheduling policy name" FORCE)
endif (CONFIG_BBQUE_SCHEDPOL_DEFAULT_GRIDBALANCE)

set(PLUGIN_GRIDBALANCE_SRC gridbalance_schedpol gridbalance_plugin)

add_library(bbque_schedpol_gridbalance MODULE ${PLUGIN_GRIDBALANCE_SRC})

target_link_libraries(
	bbque_schedpol_gridbalance
	${Boost_LIBRARIES}
)

install(TARGETS bbque_schedpol_gridbalance LIBRARY
		DESTINATION ${BBQUE_PATH_PLUGINS}
		COMPONENT BarbequeRTRM)
