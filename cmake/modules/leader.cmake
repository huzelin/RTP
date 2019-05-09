file(GLOB_RECURSE leader_source "leader/*.cc")

file(GLOB_RECURSE leader_proto_files "leader/proto/*.proto")
protobuf_generate_cpp_py(
    ${PROJECT_BINARY_DIR}/proto/
    proto_srcs
    proto_hdrs
    proto_python
    "${PROJECT_SOURCE_DIR}/leader"
    "proto"
    ${leader_proto_files})
list(APPEND leader_source ${proto_srcs})
include_directories(
    ${PROJECT_BINARY_DIR}/proto/
)

file(GLOB_RECURSE leader_proto_files "leader/proto/*.proto")
