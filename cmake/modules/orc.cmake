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

add_executable(orc_main "orc/framework/orc_main.cc")
target_link_libraries(orc_main rtp_core)

# Copy Test Data
add_custom_target(orc_utest_data ALL DEPENDS)
add_custom_command(TARGET orc_utest_data
    POST_BUILD
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/orc/framework/testdata/ ${CMAKE_BINARY_DIR}/ COMMENT "copy orc utest data"
    COMMAND cp -r ${CMAKE_SOURCE_DIR}/orc/workflow/testdata/ ${CMAKE_BINARY_DIR}/ COMMENT "copy orc utest data")
