rtp_option(QED_USE_AVX2 "qed use avx256" ON)
rtp_option(QED_USE_AVX512 "qed use avx512" OFF)

add_compile_options(-mf16c)
if(QED_USE_AVX2)
    add_compile_options(-mavx2)
endif()
if(QED_USE_AVX512)
    add_compile_options(-mavx512f)
endif()


file(GLOB_RECURSE qed_source "qed/*.cc")
file(GLOB_RECURSE qed_test_source "qed/*_test.cc")

list(REMOVE_ITEM qed_source ${qed_test_source})
foreach(source ${qed_test_source})
    rtp_add_test(${source} "tests" rtp_core gtest)
endforeach()
