add_library(openssl STATIC IMPORTED GLOBAL)
set_target_properties(openssl PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/openssl/openssl_1_0_2/lib64/libssl.a)

add_library(crypto STATIC IMPORTED GLOBAL)
set_target_properties(crypto PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/openssl/openssl_1_0_2/lib64/libcrypto.a)
