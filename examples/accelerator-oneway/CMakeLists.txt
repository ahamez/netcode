add_executable(accelerator-oneway-sender sender.cc)
target_link_libraries(accelerator-oneway-sender ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

add_executable(accelerator-oneway-receiver receiver.cc)
target_link_libraries(accelerator-oneway-receiver ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
