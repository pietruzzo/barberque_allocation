
#----- Add "SCALLOP" target dynamic library

if (NOT CONFIG_BBQUE_SCHEDPOL_SCALLOP)
	return(scallop)
endif(NOT CONFIG_BBQUE_SCHEDPOL_SCALLOP)

# Set the macro for the scheduling policy loading
if (CONFIG_SCHEDPOL_DEFAULT_SCALLOP)
  set (BBQUE_SCHEDPOL_DEFAULT "scallop" CACHE STRING
	  "Setting scheduling policy name" FORCE)
endif (CONFIG_SCHEDPOL_DEFAULT_SCALLOP)

set(PLUGIN_SCALLOP_SRC scallop_schedpol scallop_plugin)

add_library(bbque_schedpol_scallop MODULE ${PLUGIN_SCALLOP_SRC})

target_link_libraries(
	bbque_schedpol_scallop
	${Boost_LIBRARIES}
)

install(TARGETS bbque_schedpol_scallop LIBRARY
		DESTINATION ${BBQUE_PATH_PLUGINS}
		COMPONENT BarbequeRTRM)
