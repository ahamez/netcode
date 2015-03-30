set(
  NTC_SOURCES
  detail/coefficient.cc
  detail/decoder.cc
  detail/encoder.cc
  detail/invert_matrix.cc
)

add_library(ntc STATIC ${NTC_SOURCES})
