file(GLOB_RECURSE common_source "common/*.cc")
file(GLOB_RECURSE common_test_source "common/*_test.cc")
if(NOT ${common_test_source})
    list(REMOVE_ITEM common_source ${common_test_source})
    rtp_add_test(${common_test_source} "common" rtp_core gtest)
endif()

