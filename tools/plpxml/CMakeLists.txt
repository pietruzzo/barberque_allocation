if (CONFIG_BBQUE_PLP_XML)

	# Add "barbeque" specific flags
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++0x")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--gc-sections")

	#----- Add thereafter all the regression tests we want to run
	set(BBQUE_PLPXML_SRC plp_translator plpxml)


	#----- Add "bbque-plp" target application
	add_executable(bbque-plp ${BBQUE_PLPXML_SRC})

	target_link_libraries(bbque-plp
        	${Boost_LIBRARIES}
)


        install(TARGETS bbque-plp
                DESTINATION ${BBQUE_PATH_BBQ}
                COMPONENT BarbequeTOOLS)

endif (CONFIG_BBQUE_PLP_XML)


