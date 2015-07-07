add_executable(forward forward.cc)

add_executable(lossy_proxy lossy_proxy.cc)

add_executable(replay replay.cc)
target_link_libraries(replay ntc ${GF_COMPLETE_LIBRARY})
