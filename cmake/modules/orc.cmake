file(GLOB_RECURSE orc_source "workflow/orc/*.cc" "workflow/orc/*.c")
file(GLOB_RECURSE orc_test_source "workflow/orc/*_test.cc")

list(REMOVE_ITEM orc_source ${orc_test_source})
list(REMOVE_ITEM orc_source "workflow/orc/workflow/parser/interpret.cc")

foreach(source ${orc_test_source})
    rtp_add_test(${source} "tests" rtp_core gtest)
endforeach()

add_executable(orc_interpret "workflow/orc/workflow/parser/interpret.cc")
target_link_libraries(orc_interpret rtp_core)
