/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 * These tests are supposed to run on an x86 host.
 ******************************************************************************/

#include <gtest/gtest.h>

#include <bit>
#include <cfenv>
#include <chrono>
#include <cmath>
#include <iostream>

#include "float_rng.h"
#include "floppy_float.h"
#include "utils.h"

extern "C" {
#include "softfloat/softfloat.h"
}

constexpr i32 kNumIterations = 10000000;
constexpr i32 kRngSeed = 42;

TEST(PerformanceTests, Addf32) {
  FloppyFloat ff;
  FloatRng<f32> float_rng(kRngSeed);
  ff.SetupTox86();

  f32 valuefa{float_rng.Gen()};
  float32_t valuesfa{std::bit_cast<u32>(valuefa)};
  f32 valuefb{float_rng.Gen()};
  float32_t valuesfb{std::bit_cast<u32>(valuefb)};

  std::array<int, 4> rounding_modes = {::softfloat_round_near_even, ::softfloat_round_minMag, ::softfloat_round_min,
                                       ::softfloat_round_max};

  std::chrono::steady_clock::time_point begin;
  std::chrono::steady_clock::time_point end;

  begin = std::chrono::steady_clock::now();
  for (auto rm : rounding_modes) {
    for (size_t i = 0; i < kNumIterations; ++i) {
      [[maybe_unused]] f32 ff_result;

      switch (rm) {
      case ::softfloat_round_near_even:
        ff_result = ff.Add<f32, FloppyFloat::kRoundTiesToEven>(valuefa, valuefb);
        break;
      case ::softfloat_round_minMag:
        ff_result = ff.Add<f32, FloppyFloat::kRoundTowardZero>(valuefa, valuefb);
        break;
      case ::softfloat_round_min:
        ff_result = ff.Add<f32, FloppyFloat::kRoundTowardNegative>(valuefa, valuefb);
        break;
      case ::softfloat_round_max:
        ff_result = ff.Add<f32, FloppyFloat::kRoundTowardPositive>(valuefa, valuefb);
        break;
      default:
        break;
      }
      ff.ClearFlags();

      valuesfb = valuesfa;
      valuefa = float_rng.Gen();
    }
    float_rng.Reset();
  }
  end = std::chrono::steady_clock::now();
  auto ms_ff_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  std::cout << "Milliseconds FloppyFloat: " << ms_ff_float << std::endl;

  begin = std::chrono::steady_clock::now();
  for (auto rm : rounding_modes) {
    ::softfloat_roundingMode = rm;
    for (size_t i = 0; i < kNumIterations; ++i) {
      float32_t sf_result;

      sf_result = ::f32_add(valuesfa, valuesfb);

      ::softfloat_exceptionFlags = 0;

      valuesfb.v = valuesfa.v;
      valuesfa.v = std::bit_cast<u32>(float_rng.Gen());
    }
    float_rng.Reset();
  }
  end = std::chrono::steady_clock::now();
  auto ms_sf_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  std::cout << "Milliseconds SoftFloat: " << ms_sf_float << std::endl;

  ASSERT_GE(ms_sf_float, ms_ff_float);
}


TEST(PerformanceTests, Mulf64) {
  FloppyFloat ff;
  FloatRng<f64> float_rng(kRngSeed);
  ff.SetupTox86();

  f64 valuefa{float_rng.Gen()};
  float64_t valuesfa{std::bit_cast<u64>(valuefa)};
  f64 valuefb{float_rng.Gen()};
  float64_t valuesfb{std::bit_cast<u64>(valuefb)};

  std::array<int, 4> rounding_modes = {::softfloat_round_near_even, ::softfloat_round_minMag, ::softfloat_round_min,
                                       ::softfloat_round_max};

  std::chrono::steady_clock::time_point begin;
  std::chrono::steady_clock::time_point end;

  begin = std::chrono::steady_clock::now();
  for (auto rm : rounding_modes) {
    for (size_t i = 0; i < kNumIterations; ++i) {
      [[maybe_unused]] f64 ff_result;

      switch (rm) {
      case ::softfloat_round_near_even:
        ff_result = ff.Mul<f64, FloppyFloat::kRoundTiesToEven>(valuefa, valuefb);
        break;
      case ::softfloat_round_minMag:
        ff_result = ff.Mul<f64, FloppyFloat::kRoundTowardZero>(valuefa, valuefb);
        break;
      case ::softfloat_round_min:
        ff_result = ff.Mul<f64, FloppyFloat::kRoundTowardNegative>(valuefa, valuefb);
        break;
      case ::softfloat_round_max:
        ff_result = ff.Mul<f64, FloppyFloat::kRoundTowardPositive>(valuefa, valuefb);
        break;
      default:
        break;
      }
      ff.ClearFlags();

      valuesfb = valuesfa;
      valuefa = float_rng.Gen();
    }
    float_rng.Reset();
  }
  end = std::chrono::steady_clock::now();
  auto ms_ff_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  std::cout << "Milliseconds FloppyFloat: " << ms_ff_float << std::endl;

  begin = std::chrono::steady_clock::now();
  for (auto rm : rounding_modes) {
    ::softfloat_roundingMode = rm;
    for (size_t i = 0; i < kNumIterations; ++i) {
      float64_t sf_result;

      sf_result = ::f64_mul(valuesfa, valuesfb);

      ::softfloat_exceptionFlags = 0;

      valuesfb.v = valuesfa.v;
      valuesfa.v = std::bit_cast<u64>(float_rng.Gen());
    }
    float_rng.Reset();
  }
  end = std::chrono::steady_clock::now();
  auto ms_sf_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  std::cout << "Milliseconds SoftFloat: " << ms_sf_float << std::endl;

  ASSERT_GE(ms_sf_float, ms_ff_float);
}


int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}