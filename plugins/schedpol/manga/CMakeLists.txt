
#----- Add "MANGA" target dynamic library

if (NOT CONFIG_BBQUE_SCHEDPOL_MANGA)
	return(manga)
endif(NOT CONFIG_BBQUE_SCHEDPOL_MANGA)

# Set the macro for the scheduling policy loading
if (CONFIG_BBQUE_SCHEDPOL_DEFAULT_MANGA)
  set (BBQUE_SCHEDPOL_DEFAULT "manga" CACHE STRING
	  "Setting scheduling policy name" FORCE)
endif (CONFIG_BBQUE_SCHEDPOL_DEFAULT_MANGA)

set(PLUGIN_MANGA_SRC manga_schedpol manga_plugin)

add_library(bbque_schedpol_manga MODULE ${PLUGIN_MANGA_SRC})

target_link_libraries(
	bbque_schedpol_manga
	${Boost_LIBRARIES}
)

install(TARGETS bbque_schedpol_manga LIBRARY
		DESTINATION ${BBQUE_PATH_PLUGINS}
		COMPONENT BarbequeRTRM)
