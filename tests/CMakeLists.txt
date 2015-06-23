set(SOURCES
   tests.cc
   netcode/c/test_data.cc
   netcode/detail/test_buffer.cc
   netcode/detail/test_decoder.cc
   netcode/detail/test_encoder.cc
   netcode/detail/test_galois_field.cc
   netcode/detail/test_invert_matrix.cc
   netcode/detail/test_packetizer.cc
   netcode/detail/test_source_list.cc
   netcode/detail/test_square_matrix.cc
   netcode/test_data.cc
   netcode/test_decoder.cc
   netcode/test_encoder.cc
   netcode/test_reconstruction.cc
   )

add_executable(tests ${SOURCES})
target_link_libraries(tests ntc cntc ${GF_COMPLETE_LIBRARY})


set(END_TO_END_SOURCES
    end_to_end.cc
    )
add_executable(end_to_end ${END_TO_END_SOURCES})
target_link_libraries(end_to_end ntc ${GF_COMPLETE_LIBRARY})


set(END_TO_END_MT_SOURCES
    end_to_end_mt.cc
    )
add_executable(end_to_end_mt ${END_TO_END_MT_SOURCES})
target_link_libraries(end_to_end_mt ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

add_test(Tests tests)
