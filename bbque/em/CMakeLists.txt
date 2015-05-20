
# Add EventManager main class
set (EVENT_MANAGER_SRC event event_wrapper event_manager)

# Add as library
add_library(bbque_em STATIC ${EVENT_MANAGER_SRC})

# Link static library
target_link_libraries(
	bbque_em
	${Boost_LIBRARIES}
)
