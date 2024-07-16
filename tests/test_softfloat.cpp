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

  u16 v = 0;
  do {
    ++v;
    float16_t sf;
    sf.v = v;
    f16 vf16;
    vf16 = std::bit_cast<f16>(v);
    if (std::isnan(vf16)) {
      vf16 = std::numeric_limits<f16>::quiet_NaN();
      sf.v = std::bit_cast<u16>(vf16);
    }
    u16 ff_value = std::bit_cast<u16>(ff.Add<f16>(vf16, vf16));
    u16 sf_value = f16_add(sf, sf).v;
    ASSERT_EQ(ff_value, sf_value);

    // Excpetion Flags
    ASSERT_EQ(ff.invalid, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_invalid));
    ASSERT_EQ(ff.division_by_zero, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_infinite));
    ASSERT_EQ(ff.overflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_overflow));
    ASSERT_EQ(ff.underflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_underflow));
    ASSERT_EQ(ff.inexact, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_inexact));

    softfloat_exceptionFlags = 0;
    ff.ClearFlags();

  } while (v != 0u);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}