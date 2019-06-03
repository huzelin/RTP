add_library(3rdparty_boost INTERFACE)

add_library(3rdparty_boost_date_time STATIC IMPORTED)
set_target_properties(3rdparty_boost_date_time PROPERTIES IMPORTED_LOCATION
	${PROJECT_SOURCE_DIR}/3rdparty/boost/boost-1.16.0/lib64/libboost_date_time.a)

add_library(3rdparty_boost_filesystem STATIC IMPORTED)
set_target_properties(3rdparty_boost_filesystem PROPERTIES IMPORTED_LOCATION
	${PROJECT_SOURCE_DIR}/3rdparty/boost/boost-1.16.0/lib64/libboost_filesystem.a)

add_library(3rdparty_boost_random STATIC IMPORTED)
set_target_properties(3rdparty_boost_random PROPERTIES IMPORTED_LOCATION
	${PROJECT_SOURCE_DIR}/3rdparty/boost/boost-1.16.0/lib64/libboost_random.a)

add_library(3rdparty_boost_system STATIC IMPORTED)
set_target_properties(3rdparty_boost_system PROPERTIES IMPORTED_LOCATION
	${PROJECT_SOURCE_DIR}/3rdparty/boost/boost-1.16.0/lib64/libboost_system.a)

add_library(3rdparty_boost_graph STATIC IMPORTED)
set_target_properties(3rdparty_boost_graph PROPERTIES IMPORTED_LOCATION
	${PROJECT_SOURCE_DIR}/3rdparty/boost/boost-1.16.0/lib64/libboost_graph.a)

add_library(3rdparty_boost_regex STATIC IMPORTED)
set_target_properties(3rdparty_boost_regex PROPERTIES IMPORTED_LOCATION
	${PROJECT_SOURCE_DIR}/3rdparty/boost/boost-1.16.0/lib64/libboost_regex.a)

add_library(3rdparty_boost_program_options STATIC IMPORTED)
set_target_properties(3rdparty_boost_program_options PROPERTIES IMPORTED_LOCATION
	${PROJECT_SOURCE_DIR}/3rdparty/boost/boost-1.16.0/lib64/libboost_program_options.a)

add_library(3rdparty_boost_thread STATIC IMPORTED)
set_target_properties(3rdparty_boost_thread PROPERTIES IMPORTED_LOCATION
	${PROJECT_SOURCE_DIR}/3rdparty/boost/boost-1.16.0/lib64/libboost_thread.a)

target_link_libraries(3rdparty_boost
	INTERFACE 3rdparty_boost_date_time
	INTERFACE 3rdparty_boost_filesystem
	INTERFACE 3rdparty_boost_random
	INTERFACE 3rdparty_boost_system
	INTERFACE 3rdparty_boost_graph
	INTERFACE 3rdparty_boost_regex
	INTERFACE 3rdparty_boost_program_options
	INTERFACE 3rdparty_boost_thread
	)

