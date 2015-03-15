set(SOURCES
   tests.cc
   netcode/detail/protocol/simple.cc
   netcode/encoder.cc
   netcode/symbol.cc
   )

add_executable(tests ${SOURCES})
