add_library(brpc STATIC IMPORTED GLOBAL)
set_target_properties(brpc PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/brpc/brpc-0.9.5/lib64/libbrpc.a)
