project(RTP)

cmake_minimum_required(VERSION 2.8)

option(test "Build all tests" ON)
enable_testing()
set(CMAKE_VERBOSE_MAKEFILE ON)

include(cmake/Utils.cmake)
include(cmake/ProtoBuf.cmake)

include_directories(
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/
    ${PROJECT_SOURCE_DIR}/3rdparty/
    ${PROJECT_SOURCE_DIR}/3rdparty/zookeeper/zookeeper-3.4.14/include
    ${PROJECT_SOURCE_DIR}/3rdparty/grpc/grpc-1.20.0/include
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG -std=c++11 -fPIC -ftree-vectorize -Wcpp -g")

# Add modules
include(cmake/modules/common.cmake)
include(cmake/modules/qed.cmake)
include(cmake/modules/leader.cmake)

add_subdirectory(3rdparty)

list(APPEND SOURCE
    ${common_source}
    ${qed_source}
    ${leader_source}
)
add_library(rtp_core STATIC ${SOURCE})
target_link_libraries(rtp_core
    zookeeper
    grpc++
    protobuf
    pthread
    rt)

