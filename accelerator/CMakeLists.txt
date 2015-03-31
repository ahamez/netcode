set(CLIENT_SOURCES client.cc)

add_executable(client ${CLIENT_SOURCES})
target_link_libraries(client ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

set(SERVER_SOURCES server.cc)

add_executable(server ${SERVER_SOURCES})
target_link_libraries(server ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

