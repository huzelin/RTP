add_library(zookeeper STATIC IMPORTED GLOBAL)
set_target_properties(zookeeper PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/zookeeper/zookeeper-3.4.14/lib64/libzookeeper_mt.a)
