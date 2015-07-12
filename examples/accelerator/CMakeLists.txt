include_directories("${PROJECT_SOURCE_DIR}/examples")
add_executable(accelerator accelerator.cc)
target_link_libraries(accelerator ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
