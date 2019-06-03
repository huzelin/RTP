file(GLOB_RECURSE orc_source "orc/*.cc" "orc/*.c")
file(GLOB_RECURSE orc_test_source "orc/*_test.cc")

list(REMOVE_ITEM orc_source ${orc_test_source})
file(GLOB_RECURSE orc_main_source "orc/*_main.cc")
list(REMOVE_ITEM orc_source ${orc_main_source})

foreach(source ${orc_test_source})
    rtp_add_test(${source} "tests" rtp_core gtest)
endforeach()

add_executable(orc_interpret "orc/workflow/parser/interpret_main.cc")
target_link_libraries(orc_interpret rtp_core)

add_library(orc_main STATIC "orc/framework/orc_main.cc")
target_link_libraries(orc_main rtp_core)

# Copy Test Data
add_custom_target(orc_utest_data ALL DEPENDS)
add_custom_command(TARGET orc_utest_data
    POST_BUILD
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/orc/framework/testdata/ ${CMAKE_BINARY_DIR}/ COMMENT "copy orc utest data"
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/orc/workflow/testdata/ ${CMAKE_BINARY_DIR}/ COMMENT "copy orc utest data")

# Add example of ORC.
file(GLOB_RECURSE orc_example_proto_files "example/orc/*.proto")
protobuf_generate_cpp_py(
    ${PROJECT_BINARY_DIR}/orc/
    orc_example_proto_srcs
    orc_example_proto_hdrs
    orc_example_proto_python
    "${PROJECT_SOURCE_DIR}/example"
    "orc"
    ${orc_example_proto_files})
file(GLOB_RECURSE orc_example_source_serv "example/orc/server/*.cc")
file(GLOB_RECURSE orc_example_source_cli "example/orc/client/*.cc")
list(APPEND orc_example_source_serv ${orc_example_proto_srcs})
list(APPEND orc_example_source_cli ${orc_example_proto_srcs})
include_directories(${PROJECT_BINARY_DIR}/orc)

add_executable(orc_example_source_serv ${orc_example_source_serv})
target_link_libraries(orc_example_source_serv orc_main)

add_executable(orc_example_source_cli ${orc_example_source_cli})
target_link_libraries(orc_example_source_cli rtp_core)
