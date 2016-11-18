
#----- Add "TEST" target dynamic library

if (NOT CONFIG_BBQUE_SCHEDPOL_TEST)
	return(test)
endif(NOT CONFIG_BBQUE_SCHEDPOL_TEST)

# Set the macro for the scheduling policy loading
if (CONFIG_SCHEDPOL_DEFAULT_TEST)
  set (BBQUE_SCHEDPOL_DEFAULT "test" CACHE STRING
	  "Setting scheduling policy name" FORCE)
endif (CONFIG_SCHEDPOL_DEFAULT_TEST)

set(PLUGIN_TEST_SRC test_schedpol test_plugin)

add_library(bbque_schedpol_test MODULE ${PLUGIN_TEST_SRC})

target_link_libraries(
	bbque_schedpol_test
	${Boost_LIBRARIES}
)

install(TARGETS bbque_schedpol_test LIBRARY
		DESTINATION ${BBQUE_PATH_PLUGINS}
		COMPONENT BarbequeRTRM)
