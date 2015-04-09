set(ACCELERATOR_SOURCES accelerator.cc)

add_executable(accelerator ${ACCELERATOR_SOURCES})
target_link_libraries(accelerator ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

