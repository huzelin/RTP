add_library(openssl STATIC IMPORTED GLOBAL)
set_target_properties(openssl PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/openssl/openssl_1_1_1/lib64/libssl.a)
