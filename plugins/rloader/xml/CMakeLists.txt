
if (NOT CONFIG_BBQUE_RLOADER_XML)
	return(xml)
endif(NOT CONFIG_BBQUE_RLOADER_XML)

# Set the macro for the scheduling policy loading
if (CONFIG_BBQUE_RLOADER_DEFAULT_YAMS)
  set (BBQUE_RLOADER_DEFAULT "xml" CACHE STRING
	  "Setting scheduling policy name" FORCE)
endif (CONFIG_BBQUE_RLOADER_DEFAULT_YAMS)


#----- Check for required TiCPP library
find_package(TiCPP REQUIRED)

# Add compilation dependencies
include_directories(${ticpp_INCLUDE_DIRS})

# Add linking dependencies
link_directories(${ticpp_LIBRARY_DIRS})

#----- Add target static library
set(PLUGIN_RECIPELOAD_XML_SRC  xml_rloader xml_plugin)
add_library(bbque_recipe_loader_xml STATIC ${PLUGIN_RECIPELOAD_XML_SRC})

#----- Add library specific flags
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--gc-sections")

#----- Linking dependencies
target_link_libraries(bbque_recipe_loader_xml ${TICPP_LIBRARIES})

