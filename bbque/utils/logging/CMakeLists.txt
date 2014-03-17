
# Add sources in the current directory to the target binary
set (BBQUE_LOGGER_SRC console_logger)
if (CONFIG_EXTERNAL_LOG4CPP)
	set (BBQUE_LOGGER_SRC ${BBQUE_LOGGER_SRC} log4cpp_logger)
endif (CONFIG_EXTERNAL_LOG4CPP)
if (CONFIG_TARGET_ANDROID)
	set (BBQUE_LOGGER_SRC ${BBQUE_LOGGER_SRC} android_logger)
endif (CONFIG_TARGET_ANDROID)

#Add as library
add_library(bbque_logger STATIC ${BBQUE_LOGGER_SRC})

