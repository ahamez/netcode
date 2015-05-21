set(
  NTC_SOURCES
  detail/decoder.cc
  detail/encoder.cc
  detail/invert_matrix.cc
)

set(
  CNTC_SOURCES
  c/configuration.cc
  c/data.cc
  c/decoder.cc
  c/encoder.cc
)


add_library(ntc STATIC ${NTC_SOURCES})
add_library(cntc STATIC ${CNTC_SOURCES})

install(TARGETS ntc cntc DESTINATION lib)
install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/netcode DESTINATION include
  FILES_MATCHING PATTERN "*.hh" PATTERN "*.h"
)
