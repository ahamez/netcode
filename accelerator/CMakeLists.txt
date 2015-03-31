set(ENCODER_SOURCES encoder.cc)

add_executable(encoder ${ENCODER_SOURCES})
target_link_libraries(encoder ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

set(DECODER_SOURCES decoder.cc)

add_executable(decoder ${DECODER_SOURCES})
target_link_libraries(decoder ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

