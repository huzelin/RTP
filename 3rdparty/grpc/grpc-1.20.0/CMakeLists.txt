# C++
add_library(grpc++_core_stats STATIC IMPORTED GLOBAL)
set_target_properties(grpc++_core_stats PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/lib64/libgrpc++_core_stats.a)

add_library(grpc++_cronet STATIC IMPORTED GLOBAL)
set_target_properties(grpc++_cronet PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/lib64/libgrpc++_cronet.a)

add_library(grpc++_error_details STATIC IMPORTED GLOBAL)
set_target_properties(grpc++_error_details PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/lib64/libgrpc++_error_details.a)

add_library(grpc++_reflection STATIC IMPORTED GLOBAL)
set_target_properties(grpc++_reflection PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/lib64/libgrpc++_reflection.a)

add_library(grpc++_unsecure STATIC IMPORTED GLOBAL)
set_target_properties(grpc++_unsecure PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/lib64/libgrpc++_unsecure.a)

add_library(grpc++ STATIC IMPORTED GLOBAL)
set_target_properties(grpc++ PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/lib64/libgrpc++.a)

# C
add_library(grpc STATIC IMPORTED GLOBAL)
set_target_properties(grpc PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/lib64/libgrpc.a)
