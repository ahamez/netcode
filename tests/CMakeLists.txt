set(SOURCES
   tests.cc
   netcode/c/test_data.cc
   netcode/c/test_decoder.cc
   netcode/detail/test_buffer.cc
   netcode/detail/test_decoder.cc
   netcode/detail/test_encoder.cc
   netcode/detail/test_galois_field.cc
   netcode/detail/test_invert_matrix.cc
   netcode/detail/test_packetizer.cc
   netcode/detail/test_source_list.cc
   netcode/detail/test_square_matrix.cc
   netcode/test_decoder.cc
   netcode/test_encoder.cc
   netcode/test_packet.cc
   netcode/test_reconstruction.cc
   )

add_executable(tests ${SOURCES})
target_link_libraries(tests ntc cntc ${GF_COMPLETE_LIBRARY})

add_executable(end_to_end end_to_end.cc)
target_link_libraries(end_to_end ntc ${GF_COMPLETE_LIBRARY})

add_executable(end_to_end_mt end_to_end_mt.cc)
target_link_libraries(end_to_end_mt ntc ${GF_COMPLETE_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

add_test(Tests tests)
add_test(EndToEnd end_to_end 1000)
add_test(EndToEnd-MT end_to_end_mt 2)
