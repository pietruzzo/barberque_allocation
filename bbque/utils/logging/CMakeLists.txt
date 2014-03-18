
#----- Add basic Console based logger
set (BBQUE_LOGGER_SRC logger console_logger)

#----- Generic Linux settings
if (CONFIG_EXTERNAL_LOG4CPP)
	set (BBQUE_LOGGER_SRC ${BBQUE_LOGGER_SRC} log4cpp_logger)

	#----- Check for required LOG4CPP library
	find_package(Log4CPP REQUIRED)

	# Add compilation dependencies
	include_directories(${log4cpp_INCLUDE_DIRS})

	# Add linking dependencies
	link_directories(${log4cpp_LIBRARY_DIRS})

	#----- Log4CPP Library linking
	set (BBQUE_LOGGER_LIBS ${LOG4CPP_LIBRARIES})

endif (CONFIG_EXTERNAL_LOG4CPP)

#----- Android specific settings
if (CONFIG_TARGET_ANDROID)
	set (BBQUE_LOGGER_SRC ${BBQUE_LOGGER_SRC} android_logger)
endif (CONFIG_TARGET_ANDROID)

#----- Add static library
add_library(bbque_logger STATIC ${BBQUE_LOGGER_SRC})

#----- Add library specific flags
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--gc-sections")

#----- Linking dependencies
if (BBQUE_LOGGER_LIBS)
	target_link_libraries(bbque_logger ${BBQUE_LOGGER_LIBS})
endif (BBQUE_LOGGER_LIBS)
