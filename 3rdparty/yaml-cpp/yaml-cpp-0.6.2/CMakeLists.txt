add_library(yaml-cpp STATIC IMPORTED GLOBAL)
set_target_properties(yaml-cpp PROPERTIES IMPORTED_LOCATION
    ${CMAKE_SOURCE_DIR}/3rdparty/yaml-cpp/yaml-cpp-0.6.2/lib64/libyaml-cpp.a)


