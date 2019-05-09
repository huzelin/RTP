add_library(grpc STATIC IMPORTED GLOBAL)
set_target_properties(grpc PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/lib64/libgrpc.a)

add_library(grpc++ STATIC IMPORTED GLOBAL)
set_target_properties(grpc++ PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/lib64/libgrpc++.a)
