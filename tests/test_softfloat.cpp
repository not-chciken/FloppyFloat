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

constexpr i32 kNumIterations = 20000;
constexpr i32 kRngSeed = 42;

TEST(SoftFloatTests, Addf16) {
  FloppyFloat ff;
  ff.SetupTox86();

  std::array<int, 4> rounding_modes = {::softfloat_round_near_even, ::softfloat_round_minMag, ::softfloat_round_min,
                                       ::softfloat_round_max};

  for (auto rm : rounding_modes) {
    ::softfloat_roundingMode = rm;
    softfloat_exceptionFlags = 0;

    FloatRng<f16> float_rng(kRngSeed);
    f16 valuefa{float_rng.Gen()};
    float16_t valuesfa{std::bit_cast<u16>(valuefa)};
    f16 valuefb{float_rng.Gen()};
    float16_t valuesfb{std::bit_cast<u16>(valuefb)};

    for (i16 i = 0; i < kNumIterations; ++i) {
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
      default:
        break;
      }

      u16 sf_result = f16_add(valuesfa, valuesfb).v;
      // std::cout << i << " " << rm << std::endl;

      ASSERT_EQ(ff_result, sf_result);
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
}

TEST(SoftFloatTests, Addf32) {
  FloppyFloat ff;
  FloatRng<f32> float_rng(kRngSeed);
  ff.SetupTox86();

  std::array<int, 4> rounding_modes = {::softfloat_round_near_even, ::softfloat_round_minMag, ::softfloat_round_min,
                                       ::softfloat_round_max};

  for (auto rm : rounding_modes) {
    ::softfloat_roundingMode = rm;
    softfloat_exceptionFlags = 0;

    f32 valuefa{float_rng.Gen()};
    float32_t valuesfa{std::bit_cast<u32>(valuefa)};
    f32 valuefb{float_rng.Gen()};
    float32_t valuesfb{std::bit_cast<u32>(valuefb)};

    for (i32 i = 0; i < kNumIterations; ++i) {
      u32 ff_result;

      switch (rm) {
      case ::softfloat_round_near_even:
        ff_result = std::bit_cast<u32>(ff.Add<f32, FloppyFloat::kRoundTiesToEven>(valuefa, valuefb));
        break;
      case ::softfloat_round_minMag:
        ff_result = std::bit_cast<u32>(ff.Add<f32, FloppyFloat::kRoundTowardZero>(valuefa, valuefb));
        break;
      case ::softfloat_round_min:
        ff_result = std::bit_cast<u32>(ff.Add<f32, FloppyFloat::kRoundTowardNegative>(valuefa, valuefb));
        break;
      case ::softfloat_round_max:
        ff_result = std::bit_cast<u32>(ff.Add<f32, FloppyFloat::kRoundTowardPositive>(valuefa, valuefb));
        break;
      default:
        break;
      }

      u32 sf_result = f32_add(valuesfa, valuesfb).v;

      ASSERT_EQ(ff_result, sf_result);
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
      valuesfa.v = std::bit_cast<u32>(valuefa);
    }
  }
}

// TEST(SoftFloatTests, Subf16) {
//   FloppyFloat ff;
//   FloatRng<f16> float_rng(42);
//   ff.nan_propagation_scheme = FloppyFloat::NanPropX86sse;
//   ff.SetQnan<f16>(0xfe00u);

//   ::softfloat_roundingMode = ::softfloat_round_near_even;
//   softfloat_exceptionFlags = 0;

//   f16 valuefa{float_rng.Gen()};
//   float16_t valuesfa{std::bit_cast<u16>(valuefa)};
//   f16 valuefb{float_rng.Gen()};
//   float16_t valuesfb{std::bit_cast<u16>(valuefb)};

//   for (size_t i = 0; i < 20000u; ++i) {
//     u16 ff_result = std::bit_cast<u16>(ff.Sub<f16, FloppyFloat::kRoundTiesToEven>(valuefa, valuefb));
//     u16 sf_result = f16_sub(valuesfa, valuesfb).v;
//     ASSERT_EQ(ff_result, sf_result);

//     // Excpetion Flags
//     ASSERT_EQ(ff.invalid, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_invalid));
//     ASSERT_EQ(ff.division_by_zero, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_infinite));
//     ASSERT_EQ(ff.overflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_overflow));
//     ASSERT_EQ(ff.underflow, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_underflow));
//     ASSERT_EQ(ff.inexact, static_cast<bool>(::softfloat_exceptionFlags & softfloat_flag_inexact));

//     softfloat_exceptionFlags = 0;
//     ff.ClearFlags();

//     valuesfb = valuesfa;
//     valuefb = valuefa;
//     valuefa = float_rng.Gen();
//     valuesfa.v = std::bit_cast<u16>(valuefa);
//   }
// }

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}