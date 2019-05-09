add_library(protobuf STATIC IMPORTED GLOBAL)
set_target_properties(protobuf PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/protobuf/protobuf-3.7.0/lib64/libprotobuf.a)

add_library(protobuf-lite STATIC IMPORTED GLOBAL)
set_target_properties(protobuf-lite PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/protobuf/protobuf-3.7.0/lib64/libprotobuf-lite.a)
