set(SOURCES
   tests.cc
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
   netcode/test_reconstruction.cc
   netcode/test_symbol.cc
   )

add_executable(tests ${SOURCES})
target_link_libraries(tests ${GF_COMPLETE_LIBRARY})
