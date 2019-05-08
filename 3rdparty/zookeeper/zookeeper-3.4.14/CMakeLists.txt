add_library(zookeeper STATIC IMPORTED GLOBAL)
set_target_properties(zookeeper PROPERTIES IMPORTED_LOCATION
    ${CMAKE_CURRENT_DIR}/lib64/libzookeeper_mt.a)
