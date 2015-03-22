set(SOURCES
   tests.cc
   netcode/detail/protocol/test_simple.cc
   netcode/detail/test_decoder.cc
   netcode/detail/test_raw_buffer.cc
   netcode/detail/test_source_list.cc
   netcode/test_decoder.cc
   netcode/test_encoder.cc
   netcode/test_symbol.cc
   )

add_executable(tests ${SOURCES})
target_link_libraries(tests ${GF_COMPLETE_LIBRARY})
