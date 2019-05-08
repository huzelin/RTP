file(GLOB_RECURSE common_source "common/*.cc")
file(GLOB_RECURSE common_test_source "common/*_test.cc")

list(REMOVE_ITEM common_source ${common_test_source})
foreach(source ${common_test_source})
    rtp_add_test(${source} "tests" rtp_core gtest)
endforeach()
