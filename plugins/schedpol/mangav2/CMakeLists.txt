
#----- Add "MANGAV2" target dynamic library

if (NOT CONFIG_BBQUE_SCHEDPOL_MANGAV2)
	return(mangav2)
endif(NOT CONFIG_BBQUE_SCHEDPOL_MANGAV2)

# Set the macro for the scheduling policy loading
if (CONFIG_BBQUE_SCHEDPOL_DEFAULT_MANGAV2)
  set (BBQUE_SCHEDPOL_DEFAULT "mangav2" CACHE STRING
	  "Setting scheduling policy name" FORCE)
endif (CONFIG_BBQUE_SCHEDPOL_DEFAULT_MANGAV2)

set(PLUGIN_MANGAV2_SRC mangav2_schedpol mangav2_plugin)

add_library(bbque_schedpol_mangav2 MODULE ${PLUGIN_MANGAV2_SRC})

target_link_libraries(
	bbque_schedpol_mangav2
	${Boost_LIBRARIES}
)

install(TARGETS bbque_schedpol_mangav2 LIBRARY
		DESTINATION ${BBQUE_PATH_PLUGINS}
		COMPONENT BarbequeRTRM)
