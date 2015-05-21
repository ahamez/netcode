set(EXAMPLE_C_SOURCES example_c.c)

add_executable(example_c ${EXAMPLE_C_SOURCES})
target_link_libraries(example_c cntc ntc ${GF_COMPLETE_LIBRARY})
