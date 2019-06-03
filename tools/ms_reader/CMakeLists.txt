set(SRCS
  monitor_status_reader.cc
)
add_executable(ms_reader ${SRCS})
target_link_libraries(ms_reader rtp_core 3rdparty_boost)
