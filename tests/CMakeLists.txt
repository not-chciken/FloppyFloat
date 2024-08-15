######################################################
# Apache License, Version 2.0
# Copyright (c) 2024 chciken/Niko Zurstraßen
######################################################

add_custom_target(tests)

add_executable(test_utils test_utils.cpp)
add_executable(test_core test_core.cpp)
add_executable(test_full_proof test_full_proof.cpp)
add_executable(test_softfloat_arm64_default_nan test_softfloat_arm64_default_nan.cpp)
add_executable(test_softfloat_riscv test_softfloat_riscv.cpp)
add_executable(test_softfloat_x86 test_softfloat_x86.cpp)
add_executable(test_x86 test_x86.cpp)

set(TEST_INCLUDE_PATHS ${CMAKE_SOURCE_DIR}/src)
set(TEST_LIBS ${CMAKE_BINARY_DIR}/libFloppyFloatTest.a -lgtest -lgcov)
set(TEST_COMPILE_OPTS --coverage -g -O3)

macro(create_test_case test_name additional_libs)
  add_dependencies(tests ${test_name})
  add_dependencies(${test_name} floppy_float_static_test)
  target_include_directories(${test_name} PUBLIC ${TEST_INCLUDE_PATHS})
  target_link_libraries(${test_name} ${TEST_LIB_PATHS} ${additional_libs})
  target_compile_options(${test_name} PUBLIC ${TEST_COMPILE_OPTS})
  target_link_libraries(${test_name} ${TEST_LIBS})
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 300)
endmacro()

create_test_case(test_utils "")
create_test_case(test_core "")
create_test_case(test_softfloat_arm64_default_nan "-lsoftfloat-arm-default-nan")
create_test_case(test_softfloat_riscv "-lsoftfloat-riscv")
create_test_case(test_softfloat_x86 "-lsoftfloat-x64")
create_test_case(test_full_proof "-lsoftfloat-riscv")
create_test_case(test_x86 "")

# Comparions against Berkely SoftFloat
add_executable(test_softfloat test_softfloat.cpp)
add_dependencies(tests test_softfloat)
add_dependencies(test_softfloat floppy_float_static_test)
target_include_directories(test_softfloat PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_softfloat ${TEST_LIBS} -lsoftfloat-x64)
target_compile_options(test_softfloat PUBLIC ${TEST_COMPILE_OPTS})
add_test(NAME test_softfloat COMMAND test_softfloat)
set_tests_properties(test_softfloat PROPERTIES TIMEOUT 300)