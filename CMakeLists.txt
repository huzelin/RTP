project(RTP)

cmake_minimum_required(VERSION 2.8)

option(test "Build all tests" ON)
enable_testing()
set(CMAKE_VERBOSE_MAKEFILE ON)

include(cmake/Utils.cmake)

include_directories(
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/3rdparty/
    )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG -std=c++11 -fPIC -ftree-vectorize -Wcpp -g")

# Add modules
include(cmake/modules/common.cmake)
include(cmake/modules/qed.cmake)

list(APPEND SOURCE ${common_source} ${qed_source})
add_library(rtp STATIC ${SOURCE})

add_subdirectory(3rdparty)
