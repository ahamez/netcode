set(LOSSY_PROXY_SOURCES lossy_proxy.cc)

add_executable(lossy_proxy ${LOSSY_PROXY_SOURCES})
target_link_libraries(lossy_proxy ${CMAKE_THREAD_LIBS_INIT})
