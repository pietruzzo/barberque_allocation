
#----- Add "CLOVES" target dynamic library

if (NOT CONFIG_BBQUE_SCHEDPOL_CLOVES)
	return(cloves)
endif(NOT CONFIG_BBQUE_SCHEDPOL_CLOVES)

# Set the macro for the scheduling policy loading
if (CONFIG_SCHEDPOL_DEFAULT_CLOVES)
  set (BBQUE_SCHEDPOL_DEFAULT "cloves" CACHE STRING
	  "Setting scheduling policy name" FORCE)
endif (CONFIG_SCHEDPOL_DEFAULT_CLOVES)

set(PLUGIN_CLOVES_SRC cloves_schedpol cloves_plugin)

add_library(bbque_schedpol_cloves MODULE ${PLUGIN_CLOVES_SRC})

target_link_libraries(
	bbque_schedpol_cloves
	${Boost_LIBRARIES}
)

install(TARGETS bbque_schedpol_cloves LIBRARY
		DESTINATION ${BBQUE_PATH_PLUGINS}
		COMPONENT BarbequeRTRM)
