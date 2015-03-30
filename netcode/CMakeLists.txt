set(
  NTC_SOURCES
  detail/decoder.cc
  detail/encoder.cc
)

add_library(ntc STATIC ${NTC_SOURCES})
