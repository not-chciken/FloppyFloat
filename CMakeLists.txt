######################################################
# Apache License, Version 2.0
# Copyright (c) 2024 chciken/Niko Zurstraßen
######################################################

cmake_minimum_required(VERSION 3.10.1)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.1")
    message(FATAL_ERROR "Insufficient gcc version (${CMAKE_CXX_COMPILER_VERSION}). Need at least 13.1.")
  endif()
endif()

project(FloppyFloat VERSION 0.0.1)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CTest)
include(CPack)
enable_testing()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_STANDARD 23)

add_library(floppy_float STATIC OBJECT src/floppy_float.cpp src/soft_float.cpp src/vfpu.cpp)
set_property(TARGET floppy_float PROPERTY POSITION_INDEPENDENT_CODE 1)
target_compile_options(floppy_float PUBLIC -g -O3)

add_library(floppy_float_shared SHARED $<TARGET_OBJECTS:floppy_float>)
set_target_properties(floppy_float_shared PROPERTIES OUTPUT_NAME "FloppyFloat")

add_library(floppy_float_static STATIC $<TARGET_OBJECTS:floppy_float>)
set_target_properties(floppy_float_static PROPERTIES OUTPUT_NAME "FloppyFloat")

add_library(floppy_float_static_test STATIC src/floppy_float.cpp src/soft_float.cpp src/vfpu.cpp)
target_compile_options(floppy_float_static_test PUBLIC -O0 -g --coverage)
set_target_properties(floppy_float_static_test PROPERTIES OUTPUT_NAME "FloppyFloatTest")

add_subdirectory(tests)

add_custom_target(berkeley_softfloat)

set (BERKELEY_SOFTFLOAT_PATH ${CMAKE_SOURCE_DIR}/tests/berkeley-softfloat-3/)
add_dependencies(berkeley_softfloat berkeley_softfloat_x86_sse)
add_dependencies(berkeley_softfloat berkeley_softfloat_riscv)
add_dependencies(berkeley_softfloat berkeley_softfloat_arm_default_nan)

file(COPY ${BERKELEY_SOFTFLOAT_PATH}/build/Linux-x86_64-GCC/ DESTINATION ${BERKELEY_SOFTFLOAT_PATH}/build/x86_sse)
file(COPY ${BERKELEY_SOFTFLOAT_PATH}/build/Linux-x86_64-GCC/ DESTINATION ${BERKELEY_SOFTFLOAT_PATH}/build/riscv)
file(COPY ${BERKELEY_SOFTFLOAT_PATH}/build/Linux-ARM-VFPv2-GCC/ DESTINATION ${BERKELEY_SOFTFLOAT_PATH}/build/arm_default_nan)

add_custom_target(
   berkeley_softfloat_x86_sse
   COMMAND SPECIALIZE_TYPE=8086-SSE make -j`nproc`
   COMMAND cp softfloat.a ../libsoftfloat-x86-sse.a
   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/berkeley-softfloat-3/build/x86_sse
)

add_custom_target(
   berkeley_softfloat_riscv
   COMMAND SPECIALIZE_TYPE=RISCV make -j`nproc`
   COMMAND cp softfloat.a ../libsoftfloat-riscv.a
   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/berkeley-softfloat-3/build/riscv
)

add_custom_target(
   berkeley_softfloat_arm_default_nan
   COMMAND SPECIALIZE_TYPE=ARM-VFPv2-defaultNaN make -j`nproc`
   COMMAND cp softfloat.a ../libsoftfloat-arm-default-nan.a
   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/berkeley-softfloat-3/build/arm_default_nan
)
