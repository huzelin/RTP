file(GLOB_RECURSE orc_source "orc/*.cc" "orc/*.c")
file(GLOB_RECURSE orc_test_source "orc/*_test.cc")

list(REMOVE_ITEM orc_source ${orc_test_source})
list(REMOVE_ITEM orc_source "orc/workflow/parser/interpret.cc")
list(REMOVE_ITEM orc_source "orc/framework/orc_main.cc")

foreach(source ${orc_test_source})
    rtp_add_test(${source} "tests" rtp_core gtest)
endforeach()

add_executable(orc_interpret "orc/workflow/parser/interpret.cc")
target_link_libraries(orc_interpret rtp_core)

add_executable(orc_main "orc/framework/orc_main.cc")
target_link_libraries(orc_main rtp_core)
