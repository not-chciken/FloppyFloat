######################################################
# Apache License, Version 2.0
# Copyright (c) 2024 chciken/Niko Zurstraßen
######################################################

add_custom_target(tests)

add_executable(test_utils test_utils.cpp)
add_executable(test_core test_core.cpp)

set(TEST_INCLUDE_PATHS ${CMAKE_SOURCE_DIR}/src)
set(TEST_LIBS ${CMAKE_BINARY_DIR}/libFloppyFloatTest.a -lgtest -lgcov)
set(TEST_COMPILE_OPTS --coverage -g -O0)

macro(create_test_case test_name)
  add_dependencies(tests ${test_name})
  add_dependencies(${test_name} floppy_float_static_test)
  target_include_directories(${test_name} PUBLIC ${TEST_INCLUDE_PATHS})
  target_link_libraries(${test_name} ${TEST_LIB_PATHS})
  target_compile_options(${test_name} PUBLIC ${TEST_COMPILE_OPTS})
  target_link_libraries(${test_name} ${TEST_LIBS})
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 300)
endmacro()

create_test_case(test_utils)
create_test_case(test_core)