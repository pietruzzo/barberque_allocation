
if (NOT CONFIG_BBQUE_PLOADER_RXML)
	return(rxml)
endif(NOT CONFIG_BBQUE_PLOADER_RXML)

# Set the macro for the Platform Manager
if (CONFIG_BBQUE_PLOADER_DEFAULT_RXML)
  set (BBQUE_PLOADER_DEFAULT "rxml" CACHE STRING "Setting platform loader name" FORCE)
endif (CONFIG_BBQUE_PLOADER_DEFAULT_RXML)

# Sources
set(PLUGIN_RXML_SRC rxml_ploader rxml_plugin)

add_library(bbque_ploader_rxml STATIC ${PLUGIN_RXML_SRC})

target_link_libraries(bbque_ploader_rxml
    ${Boost_LIBRARIES}
)

