/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 ******************************************************************************/

#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <functional>
#include <iostream>
#include <type_traits>

#include "float_rng.h"
#include "floppy_float.h"

extern "C" {
#include "softfloat/softfloat.h"
}

using namespace std::placeholders;

constexpr i32 kNumIterations = 100000;
constexpr i32 kRngSeed = 42;

FloppyFloat ff;

template <typename T>
struct FFloatToSFloat;

template <>
struct FFloatToSFloat<f16> {
  using type = float16_t;
};
template <>
struct FFloatToSFloat<f32> {
  using type = float32_t;
};
template <>
struct FFloatToSFloat<f64> {
  using type = float64_t;
};

TEST(SoftFloatTests, Addf16) {
  FloppyFloat ff;
  ff.SetupToRiscv();

  std::array<int, 4> rounding_modes = {::softfloat_round_minMag, ::softfloat_round_min,
                                       ::softfloat_round_max, ::softfloat_round_near_maxMag};
  // std::array<int, 5> rounding_modes = {::softfloat_round_near_even, ::softfloat_round_minMag, ::softfloat_round_min,
  //                                      ::softfloat_round_max, ::softfloat_round_near_maxMag};

  for (auto rm : rounding_modes) {
    ::softfloat_roundingMode = rm;
    softfloat_exceptionFlags = 0;

    FloatRng<f16> float_rng(kRngSeed);
    f16 valuefa{float_rng.Gen()};
    float16_t valuesfa{std::bit_cast<u16>(valuefa)};
    f16 valuefb{float_rng.Gen()};
    float16_t valuesfb{std::bit_cast<u16>(valuefb)};

    for (i32 i = 0; i < 65535; ++i) {
      for (i32 j = 0; j < 65535; ++j) {
        valuefa = std::bit_cast<f16>((u16)i);
        valuefb = std::bit_cast<f16>((u16)j);
        valuesfa.v = (u16)i;
        valuesfb.v = (u16)j;
        u16 ff_result;

        switch (rm) {
        case ::softfloat_round_near_even:
          ff_result = std::bit_cast<u16>(ff.Add<f16, FloppyFloat::kRoundTiesToEven>(valuefa, valuefb));
          break;
        case ::softfloat_round_minMag:
          ff_result = std::bit_cast<u16>(ff.Add<f16, FloppyFloat::kRoundTowardZero>(valuefa, valuefb));
          break;
        case ::softfloat_round_min:
          ff_result = std::bit_cast<u16>(ff.Add<f16, FloppyFloat::kRoundTowardNegative>(valuefa, valuefb));
          break;
        case ::softfloat_round_max:
          ff_result = std::bit_cast<u16>(ff.Add<f16, FloppyFloat::kRoundTowardPositive>(valuefa, valuefb));
          break;
        case ::softfloat_round_near_maxMag:
          ff_result = std::bit_cast<u16>(ff.Add<f16, FloppyFloat::kRoundTiesToAway>(valuefa, valuefb));
          break;
        default:
          break;
        }

        u16 sf_result = f16_add(valuesfa, valuesfb).v;

        ASSERT_EQ(ff_result, sf_result) << i << " " << j << " " << rm << " " << valuefa << " " << valuefb;
        ASSERT_EQ(ff.invalid, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_invalid));
        ASSERT_EQ(ff.division_by_zero, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_infinite));
        ASSERT_EQ(ff.overflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_overflow));
        ASSERT_EQ(ff.underflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_underflow));
        ASSERT_EQ(ff.inexact, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_inexact));

        softfloat_exceptionFlags = 0;
        ff.ClearFlags();
      }
      std::cout << "Finished rm/i:" << rm << "/" << i << std::endl;
    }
  }
}


int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}