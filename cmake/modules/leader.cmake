file(GLOB_RECURSE leader_source "leader/*.cc")

file(GLOB_RECURSE leader_proto_files "leader/*.proto")
protobuf_generate_cpp_py(
    ${PROJECT_SOURCE_DIR}/leader/
    proto_srcs
    proto_hdrs
    proto_python
    "${PROJECT_SOURCE_DIR}/"
    "./"
    ${leader_proto_files})
list(APPEND leader_source ${proto_srcs})
include_directories(
    ${PROJECT_BINARY_DIR}/leader/
)

file(GLOB_RECURSE leader_proto_files "leader/proto/*.proto")
