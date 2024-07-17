/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 ******************************************************************************/

#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <iostream>

#include "float_rng.h"
#include "floppy_float.h"

extern "C" {
#include "softfloat/softfloat.h"
}

TEST(SoftFloatTests, Addf16) {
  FloppyFloat ff;
  FloatRng<f16> float_rng(42);
  ff.propagate_nan = true;
  ff.SetQnan<f16>(0xfe00u);

  f16 valuefa{float_rng.Gen()};
  float16_t valuesfa{std::bit_cast<u16>(valuefa)};
  f16 valuefb{float_rng.Gen()};
  float16_t valuesfb{std::bit_cast<u16>(valuefb)};

  for (size_t i = 0; i < 20000u; ++i) {
    u16 ff_result = std::bit_cast<u16>(ff.Add<f16>(valuefa, valuefb));
    u16 sf_result = f16_add(valuesfa, valuesfb).v;
    ASSERT_EQ(ff_result, sf_result);

    // Excpetion Flags
    ASSERT_EQ(ff.invalid, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_invalid));
    ASSERT_EQ(ff.division_by_zero, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_infinite));
    ASSERT_EQ(ff.overflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_overflow));
    ASSERT_EQ(ff.underflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_underflow));
    ASSERT_EQ(ff.inexact, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_inexact));

    softfloat_exceptionFlags = 0;
    ff.ClearFlags();

    valuesfb = valuesfa;
    valuefb = valuefa;
    valuefa = float_rng.Gen();
    valuesfa.v = std::bit_cast<u16>(valuefa);
  }
}

TEST(SoftFloatTests, Subf16) {
  FloppyFloat ff;
  FloatRng<f16> float_rng(42);
  ff.propagate_nan = true;
  ff.SetQnan<f16>(0xfe00u);

  f16 valuefa{float_rng.Gen()};
  float16_t valuesfa{std::bit_cast<u16>(valuefa)};
  f16 valuefb{float_rng.Gen()};
  float16_t valuesfb{std::bit_cast<u16>(valuefb)};

  for (size_t i = 0; i < 20000u; ++i) {
    u16 ff_result = std::bit_cast<u16>(ff.Sub<f16>(valuefa, valuefb));
    u16 sf_result = f16_sub(valuesfa, valuesfb).v;
    ASSERT_EQ(ff_result, sf_result);

    // Excpetion Flags
    ASSERT_EQ(ff.invalid, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_invalid));
    ASSERT_EQ(ff.division_by_zero, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_infinite));
    ASSERT_EQ(ff.overflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_overflow));
    ASSERT_EQ(ff.underflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_underflow));
    ASSERT_EQ(ff.inexact, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_inexact));

    softfloat_exceptionFlags = 0;
    ff.ClearFlags();

    valuesfb = valuesfa;
    valuefb = valuefa;
    valuefa = float_rng.Gen();
    valuesfa.v = std::bit_cast<u16>(valuefa);
  }
}


int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}