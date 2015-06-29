add_executable(c_basic c_basic.c)
target_link_libraries(c_basic cntc ntc ${GF_COMPLETE_LIBRARY})

add_executable(cpp_basic cpp_basic.cc)
target_link_libraries(cpp_basic ntc ${GF_COMPLETE_LIBRARY})

add_test(ExamplesCBasic c_basic)
add_test(ExamplesCPPBasic cpp_basic)
