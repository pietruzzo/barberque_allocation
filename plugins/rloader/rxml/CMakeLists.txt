

if (NOT CONFIG_BBQUE_RLOADER_RXML)
	return(rxml)
endif(NOT CONFIG_BBQUE_RLOADER_RXML)


# Set the macro for the scheduling policy loading
if (CONFIG_BBQUE_RLOADER_DEFAULT_RXML)
  set (BBQUE_RLOADER_DEFAULT "rxml" CACHE STRING "Setting recipe loader name" FORCE)
endif (CONFIG_BBQUE_RLOADER_DEFAULT_RXML)

# Sources
set(PLUGIN_RXML_SRC rxml_rloader rxml_plugin)

add_library(bbque_rloader_rxml MODULE ${PLUGIN_RXML_SRC})

target_link_libraries(
	bbque_rloader_rxml
        ${Boost_LIBRARIES}
)

install(TARGETS bbque_rloader_rxml LIBRARY
        DESTINATION ${BBQUE_PATH_PLUGINS}
        COMPONENT BarbequeRTRM)


#
##----- Check for required TiCPP library
#find_package(TiCPP REQUIRED)
#
## Add compilation dependencies
#include_directories(${ticpp_INCLUDE_DIRS})
#
## Add linking dependencies
#link_directories(${ticpp_LIBRARY_DIRS})
#
##----- Add target static library
#set(PLUGIN_RECIPELOAD_XML_SRC  xml_rloader xml_plugin)
#add_library(bbque_recipe_loader_xml STATIC ${PLUGIN_RECIPELOAD_XML_SRC})
#
##----- Add library specific flags
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--gc-sections")
#
##----- Linking dependencies
#target_link_libraries(bbque_recipe_loader_xml ${TICPP_LIBRARIES})
#
