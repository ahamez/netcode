add_executable(source_forwarder source_forwarder.cc)
target_link_libraries(source_forwarder ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

add_executable(lossy_proxy lossy_proxy.cc)
target_link_libraries(lossy_proxy ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

add_executable(replay replay.cc)
target_link_libraries(replay ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
