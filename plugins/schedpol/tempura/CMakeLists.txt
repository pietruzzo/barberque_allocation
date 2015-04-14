
#----- Add "TEMPURA" target dynamic library

if (NOT CONFIG_BBQUE_SCHEDPOL_TEMPURA)
	return(tempura)
endif(NOT CONFIG_BBQUE_SCHEDPOL_TEMPURA)

# Set the macro for the scheduling policy loading
if (CONFIG_SCHEDPOL_DEFAULT_TEMPURA)
  set (BBQUE_SCHEDPOL_DEFAULT "tempura" CACHE STRING
	  "Setting scheduling policy name" FORCE)
endif (CONFIG_SCHEDPOL_DEFAULT_TEMPURA)

set(PLUGIN_TEMPURA_SRC tempura_schedpol tempura_plugin)

add_library(bbque_schedpol_tempura MODULE ${PLUGIN_TEMPURA_SRC})

target_link_libraries(
	bbque_schedpol_tempura
	${Boost_LIBRARIES}
)

install(TARGETS bbque_schedpol_tempura LIBRARY
		DESTINATION ${BBQUE_PATH_PLUGINS}
		COMPONENT BarbequeRTRM)
