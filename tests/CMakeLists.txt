set(SOURCES
   tests.cc
   netcode/detail/protocol/simple.cc
   netcode/decoder.cc
   netcode/encoder.cc
   netcode/symbol.cc
   )

add_executable(tests ${SOURCES})
target_link_libraries(tests ${GF_COMPLETE_LIBRARY})
