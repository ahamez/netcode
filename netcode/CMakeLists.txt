set(
  NTC_SOURCES
  detail/coefficient.cc
  detail/decoder.cc
  detail/encoder.cc
  detail/invert_matrix.cc
)

add_library(ntc STATIC ${NTC_SOURCES})

set(
  CNTC_SOURCES
  c/configuration.cc
  c/data.cc
  c/decoder.cc
  c/encoder.cc
)

add_library(cntc STATIC ${CNTC_SOURCES})
