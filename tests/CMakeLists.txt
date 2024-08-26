######################################################
# Apache License, Version 2.0
# Copyright (c) 2024 chciken/Niko Zurstraßen
######################################################

add_custom_target(tests)

add_executable(test_utils test_utils.cpp)
add_executable(test_softfloat_floppyfloat_arm_default_nan test_softfloat_floppyfloat.cpp)
add_executable(test_softfloat_floppyfloat_riscv test_softfloat_floppyfloat.cpp)
add_executable(test_softfloat_floppyfloat_x86 test_softfloat_floppyfloat.cpp)
add_executable(test_softfloat_softfloat_arm_default_nan test_softfloat_softfloat.cpp)
add_executable(test_softfloat_softfloat_riscv test_softfloat_softfloat.cpp)
add_executable(test_softfloat_softfloat_x86 test_softfloat_softfloat.cpp)

set(TEST_INCLUDE_PATHS ${CMAKE_SOURCE_DIR}/src)
set(TEST_LIBS ${CMAKE_BINARY_DIR}/libFloppyFloatTest.a -lgtest -lgcov)
set(TEST_COMPILE_OPTS --coverage -g -O3)
set(TEST_LIB_PATHS -L${CMAKE_SOURCE_DIR}/tests/berkeley-softfloat-3/build/)

macro(create_test_case test_name additional_libs additional_opts)
  add_dependencies(tests ${test_name})
  add_dependencies(${test_name} floppy_float_static_test)
  add_dependencies(${test_name} berkeley_softfloat)
  target_include_directories(${test_name} PUBLIC ${TEST_INCLUDE_PATHS})
  target_link_libraries(${test_name} ${TEST_LIB_PATHS} ${additional_libs})
  target_compile_options(${test_name} PUBLIC ${TEST_COMPILE_OPTS} ${additional_opts})
  target_link_libraries(${test_name} ${TEST_LIBS})
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 300)
endmacro()

create_test_case(test_utils "" "")
create_test_case(test_softfloat_floppyfloat_arm_default_nan "-lsoftfloat-arm-default-nan" "-DARCH_ARM")
create_test_case(test_softfloat_floppyfloat_riscv "-lsoftfloat-riscv" "-DARCH_RISCV")
create_test_case(test_softfloat_floppyfloat_x86 "-lsoftfloat-x86-sse" "-DARCH_X86")
create_test_case(test_softfloat_softfloat_arm_default_nan "-lsoftfloat-arm-default-nan" "-DARCH_ARM")
create_test_case(test_softfloat_softfloat_riscv "-lsoftfloat-riscv" "-DARCH_RISCV")
create_test_case(test_softfloat_softfloat_x86 "-lsoftfloat-x86-sse" "-DARCH_X86")

# Performance Comparison
add_executable(test_performance test_performance.cpp)
add_dependencies(tests test_performance)
add_dependencies(test_performance floppy_float_static)
target_include_directories(test_performance PUBLIC ${TEST_INCLUDE_PATHS})
target_link_libraries(test_performance ${CMAKE_BINARY_DIR}/libFloppyFloat.a -lsoftfloat-riscv)
target_compile_options(test_performance PUBLIC -g -O3)
add_test(NAME test_performance COMMAND test_performance)