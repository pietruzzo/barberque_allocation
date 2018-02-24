find_package(JNI REQUIRED)

if (JNI_FOUND)
	include_directories ("${JNI_INCLUDE_DIRS}")
else ()
	message (FATAL_ERROR "Unable to find JNI.")
endif()

add_library(bbque_java_bindings MODULE
  bbque_rtlib_RTLib.cc
  bbque_rtlib_model_RTLibServices.cc
  bbque_rtlib_model_BBQueEXC.cc
  bbque_rtlib_commons.cc
  )

set_target_properties(bbque_java_bindings PROPERTIES
  OUTPUT_NAME "barbeque"
  PREFIX "lib"
  INSTALL_RPATH "${CONFIG_BOSP_RUNTIME_PATH}/lib/bbque:${CONFIG_BOSP_RUNTIME_PATH}/lib"
  )

target_link_libraries(bbque_java_bindings
  bbque_rtlib
  ${JNI_LIBRARIES}
  )

install(TARGETS bbque_java_bindings LIBRARY
  DESTINATION "${BBQUE_PATH_BINDINGS}/java"
  )
